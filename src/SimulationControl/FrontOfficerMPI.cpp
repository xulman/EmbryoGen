#include "../FrontOfficer.hpp"
#include "MPI_common.hpp"
#include <limits>
#include <thread>

namespace {
class ImplementationData {
  public:
	MPIw::Comm_raii FO_comm;
	MPIw::Comm_raii Dir_comm;
	MPIw::Comm_raii Async_request_comm;
	MPIw::Comm_raii Async_response_comm;

	std::unique_ptr<std::jthread> async_thread;
};

ImplementationData& get_data(const std::shared_ptr<void>& obj) {
	return *static_cast<ImplementationData*>(obj.get());
}

void async_thread(MPI_Comm request_comm,
                  MPI_Comm response_comm,
                  FrontOfficer* fo) {
	auto send_shadow_agent = [=](int agent_id, int dest) {
		auto ag_ptr = fo->getNearbyAgent(agent_id);
		SerializedNucleusAgent ser(*dynamic_cast<const NucleusAgent*>(ag_ptr));

		MPIw::Send_one(response_comm, ser.ID, dest,
		               async_tags::send_shadow_agent);
		MPIw::Send(response_comm, ser.type, dest,
		           async_tags::send_shadow_agent);
		MPIw::Send(response_comm, ser.sphere_centres, dest,
		           async_tags::send_shadow_agent);
		MPIw::Send(response_comm, ser.sphere_radii, dest,
		           async_tags::send_shadow_agent);
		MPIw::Send_one(response_comm, ser.geom_version, dest,
		               async_tags::send_shadow_agent);
		MPIw::Send_one(response_comm, ser.currTime, dest,
		               async_tags::send_shadow_agent);
		MPIw::Send_one(response_comm, ser.incrTime, dest,
		               async_tags::send_shadow_agent);
	};

	report::debugMessage("Async thread launched");
	while (true) {
		auto req = MPIw::Recv_one<int>(request_comm);

		switch (static_cast<async_tags>(req.status.MPI_TAG)) {
		case async_tags::shutdown:
			return;
		case async_tags::request_shadow_agent:
			send_shadow_agent(req.data, req.status.MPI_SOURCE);
			break;
		case async_tags::set_rendering_debug:
			fo->setSimulationDebugRendering(bool(req.data));
			break;
		case async_tags::set_detailed_drawing: {
			bool state =
			    MPIw::Recv_one<bool>(request_comm, req.status.MPI_SOURCE,
			                         req.status.MPI_TAG)
			        .data;
			fo->setAgentsDetailedDrawingMode(req.data, state);
		} break;
		case async_tags::set_detailed_reporting: {
			bool state =
			    MPIw::Recv_one<bool>(request_comm, req.status.MPI_SOURCE,
			                         req.status.MPI_TAG)
			        .data;
			fo->setAgentsDetailedReportingMode(req.data, state);
		} break;
		default:
			throw report::rtError("Unknown command");
		}
	}

	report::debugMessage("Async thread closed");
}

} // namespace

FrontOfficer::FrontOfficer(ScenarioUPTR s,
                           const int myPortion,
                           const int allPortions)
    : scenario(std::move(s)), ID(myPortion),
      nextFOsID((myPortion + 1) % (allPortions + 1)), FOsCount(allPortions),
      nextAvailAgentID(
          std::numeric_limits<int>::max() / allPortions * (myPortion - 1) + 1),
      implementationData(std::make_shared<ImplementationData>()) {
	scenario->declareFOcontext(myPortion);
	// enable alocation for FO images
	scenario->params->setOutputImgSpecs(scenario->params->constants.sceneOffset,
	                                    scenario->params->constants.sceneSize);

	ImplementationData& impl = get_data(implementationData);
	MPI_Comm_dup(MPI_COMM_WORLD, &impl.Dir_comm);
	MPI_Comm_dup(MPI_COMM_WORLD, &impl.Async_request_comm);
	MPI_Comm_dup(MPI_COMM_WORLD, &impl.Async_response_comm);

	// Create FO comm
	MPIw::Group_raii FO_group;
	MPI_Comm_group(MPI_COMM_WORLD, &FO_group);
	MPI_Group_excl(FO_group, 1, &RANK_DIRECTOR, &FO_group);
	MPI_Comm_create_group(MPI_COMM_WORLD, FO_group, 0, &impl.FO_comm);

	report::debugMessage("Launching async thread");
	impl.async_thread = std::make_unique<std::jthread>(
	    async_thread, impl.Async_request_comm.get(),
	    impl.Async_response_comm.get(), this);
}

FrontOfficer::~FrontOfficer() {
	ImplementationData& impl = get_data(this->implementationData);
	report::debugMessage(fmt::format("running the closing sequence"));
	report::debugMessage("Closing async thread");

	MPIw::Send_one(impl.Async_request_comm, 0, getID(),
	               static_cast<int>(async_tags::shutdown));

	report::debugMessage(
	    fmt::format("will remove {} active agents", agents.size()));
	report::debugMessage(
	    fmt::format("will remove {} shadow agents", shadowAgents.size()));
}

void FrontOfficer::init() {
	init1();
	init2();

	/** Coop with Director::init2() */
	respond_publishAgentsAABBs();
	/** End of Coop with Director::init2() */

	/** Coop with Director::init3() */
	respond_renderNextFrame();
	/** End of Coop with Director::init3() */
}

void FrontOfficer::execute() {
	report::message(
	    fmt::format("FO#{} has just started the simulation", getID()));

	const float stopTime = scenario->params->constants.stopTime;
	const float incrTime = scenario->params->constants.incrTime;
	const float expoTime = scenario->params->constants.expoTime;
	bool willRenderNextFrameFlag;

	while (currTime < stopTime) {
		// will this one end with rendering?
		willRenderNextFrameFlag =
		    currTime + incrTime >= float(frameCnt) * expoTime;

		executeInternals();
		waitHereUntilEveryoneIsHereToo();
		respond_publishAgentsAABBs();

		executeExternals();
		waitHereUntilEveryoneIsHereToo();

		respond_publishAgentsAABBs();

		executeEndSub1();

		// is this the right time to export data?
		if (willRenderNextFrameFlag) {
			// will block itself until the full rendering is complete
			respond_renderNextFrame();
			// Wait for user input to take place inside Director
			waitHereUntilEveryoneIsHereToo();
		}

		executeEndSub2();
		waitHereUntilEveryoneIsHereToo();
	}
}

void FrontOfficer::respond_publishAgentsAABBs() {
	ImplementationData& impl = get_data(implementationData);
	prepareForUpdateAndPublishAgents();
	// Sending closed agents to Director
	MPIw::Gatherv_send(impl.Dir_comm, getClosedAgents(), RANK_DIRECTOR);
	// Sending started agents to Director
	MPIw::Gatherv_send(impl.Dir_comm, getStartedAgents(), RANK_DIRECTOR);
	// Sending parental links update to Director
	MPIw::Gatherv_send(impl.Dir_comm, getParentalLinksUpdates(), RANK_DIRECTOR);

	updateAndPublishAgents();

#ifndef NDEBUG
	// Sending AABB count to director
	MPIw::Gather_send_one(impl.Dir_comm, getSizeOfAABBsList(), RANK_DIRECTOR);
#endif
	postprocessAfterUpdateAndPublishAgents();
}

void FrontOfficer::respond_renderNextFrame() {
	/** TODO optmiziation needed */
	ImplementationData& impl = get_data(implementationData);

	auto to_render = MPIw::Bcast_recv<char>(impl.Dir_comm, 3, RANK_DIRECTOR);
	SceneControls& sc = *scenario->params;
	if (to_render[0])
		sc.enableProducingOutput(sc.imgMask);
	else
		sc.disableProducingOutput(sc.imgMask);

	if (to_render[1])
		sc.enableProducingOutput(sc.imgPhantom);
	else
		sc.disableProducingOutput(sc.imgPhantom);

	if (to_render[2])
		sc.enableProducingOutput(sc.imgOptics);
	else
		sc.disableProducingOutput(sc.imgOptics);

	renderNextFrame(sc.imgMask, sc.imgPhantom, sc.imgOptics);
	std::vector mask(sc.imgMask.begin(), sc.imgMask.end());
	std::vector phantom(sc.imgPhantom.begin(), sc.imgPhantom.end());
	std::vector optics(sc.imgOptics.begin(), sc.imgOptics.end());

	MPIw::Reduce_send(impl.Dir_comm, mask, MPI_MAX, RANK_DIRECTOR);
	MPIw::Reduce_send(impl.Dir_comm, phantom, MPI_MAX, RANK_DIRECTOR);
	MPIw::Reduce_send(impl.Dir_comm, optics, MPI_MAX, RANK_DIRECTOR);
}

void FrontOfficer::waitHereUntilEveryoneIsHereToo() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

void FrontOfficer::waitFor_publishAgentsAABBs() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

void FrontOfficer::waitForAllFOs() const {
	MPIw::Barrier(get_data(implementationData).FO_comm);
}

void FrontOfficer::exchange_AABBofAgents() {
	ImplementationData& impl = get_data(implementationData);

	std::vector<AABBwithInfo> to_send;
	to_send.reserve(agents.size());
	for (const auto& [id, ag] : agents)
		to_send.push_back({id, ag->getAgentTypeID(), ag->getGeometry().version,
		                   ag->getAABB()});

	auto got = MPIw::Allgatherv(impl.FO_comm, to_send);
	for (int fo = 1; fo <= int(got.size()); ++fo)
		for (const auto& [id, type_id, geom_version, AABB] : got[fo - 1]) {
			AABBs.emplace_back(AABB, id, type_id);
			agentsAndBroadcastGeomVersions[id] = geom_version;
			registerThatThisAgentIsAtThisFO(id, fo);
		}
}

std::unique_ptr<ShadowAgent>
FrontOfficer::request_ShadowAgentCopy(const int agentID,
                                      const int FOsID) const {
	ImplementationData& impl = get_data(implementationData);
	// Send request
	MPIw::Send_one(impl.Async_request_comm, agentID, FOsID,
	               async_tags::request_shadow_agent);

	// Recieve data

	SerializedNucleusAgent ser;
	ser.ID = MPIw::Recv_one<int>(impl.Async_response_comm, FOsID,
	                             async_tags::send_shadow_agent)
	             .data;

	auto type_ = MPIw::Recv<char>(impl.Async_response_comm, FOsID,
	                              async_tags::send_shadow_agent)
	                 .data;
	ser.type = std::string(type_.begin(), type_.end());

	ser.sphere_centres =
	    MPIw::Recv<Vector3d<config::geometry::precision_t>>(
	        impl.Async_response_comm, FOsID, async_tags::send_shadow_agent)
	        .data;

	ser.sphere_radii =
	    MPIw::Recv<config::geometry::precision_t>(
	        impl.Async_response_comm, FOsID, async_tags::send_shadow_agent)
	        .data;

	ser.geom_version = MPIw::Recv_one<int>(impl.Async_response_comm, FOsID,
	                                       async_tags::send_shadow_agent)
	                       .data;

	ser.currTime = MPIw::Recv_one<float>(impl.Async_response_comm, FOsID,
	                                     async_tags::send_shadow_agent)
	                   .data;

	ser.incrTime = MPIw::Recv_one<float>(impl.Async_response_comm, FOsID,
	                                     async_tags::send_shadow_agent)
	                   .data;

	return ser.createShadowCopy();
}

// Not used, synchronization is provided by rendering frame in
// respond_renderNextFrame
void FrontOfficer::waitFor_renderNextFrame() const {}