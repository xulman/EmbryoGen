#include "../Agents/util/Serialization.hpp"
#include "../FrontOfficer.hpp"
#include "../Geometries/Geometry.hpp"
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

	report::debugMessage("Async thread launched");
	while (true) {
		auto req = MPIw::Recv_one<int>(request_comm);

		switch (static_cast<async_tags>(req.status.MPI_TAG)) {
		case async_tags::shutdown:
			return;
		case async_tags::request_shadow_agent: {
			auto bytes = serializeAgent(*fo->getLocalAgent(req.data));
			MPIw::Send(response_comm, bytes, req.status.MPI_SOURCE,
			           async_tags::send_shadow_agent);
		} break;
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
		case async_tags::request_type_string: {
			auto type_ID = MPIw::Recv_one<std::size_t>(
			                   response_comm, req.status.MPI_SOURCE,
			                   async_tags::request_type_string)
			                   .data;
			MPIw::Send(response_comm, fo->translateNameIdToAgentName(type_ID),
			           req.status.MPI_SOURCE, async_tags::send_type_string);
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
      nextAvailAgentID(std::numeric_limits<uint16_t>::max() / allPortions *
                           (myPortion - 1) +
                       1),
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

/* Optimized version of sending image to the director ... but yet still slower
:D void FrontOfficer::respond_renderNextFrame() { ImplementationData& impl =
get_data(implementationData);

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

    auto send_image = [&](const auto& img) {
        using value_type = std::remove_const_t<
            std::remove_pointer_t<decltype(img.GetFirstVoxelAddr())>>;
        i3d::Vector3d<std::size_t> min_point = img.GetSize();
        i3d::Vector3d<std::size_t> max_point(0);

        for (std::size_t x = 0; x < img.GetSizeX(); ++x)
            for (std::size_t y = 0; y < img.GetSizeY(); ++y)
                for (std::size_t z = 0; z < img.GetSizeZ(); ++z)
                    if (img.GetVoxel(x, y, z) != 0) {
                        min_point = i3d::min(min_point, {x, y, z});
                        max_point = i3d::max(max_point, {x + 1, y + 1, z + 1});
                    }

        // No valid pixel found
        if (max_point == i3d::Vector3d<std::size_t>{0, 0, 0})
            min_point = {0, 0, 0};

        assert(min_point.x <= max_point.x);
        assert(min_point.y <= max_point.y);
        assert(min_point.z <= max_point.z);

        i3d::Vector3d<std::size_t> slice_size = max_point - min_point;

        std::vector<value_type> to_send(slice_size.x * slice_size.y *
                                        slice_size.z);

        std::size_t i = 0;
        for (std::size_t x = 0; x < slice_size.x; ++x)
            for (std::size_t y = 0; y < slice_size.y; ++y)
                for (std::size_t z = 0; z < slice_size.z; ++z)
                    to_send[i++] =
                        img.GetVoxel(min_point + i3d::Vector3d{x, y, z});

        assert(i == to_send.size());

        MPIw::Gather_send(impl.Dir_comm,
                          std::array{min_point.x, min_point.y, min_point.z,
                                     slice_size.x, slice_size.y, slice_size.z},
                          RANK_DIRECTOR);

        MPIw::Gatherv_send(impl.Dir_comm, to_send, RANK_DIRECTOR);
    };

    send_image(sc.imgMask);
    send_image(sc.imgPhantom);
    send_image(sc.imgOptics);
}
*/

void FrontOfficer::waitHereUntilEveryoneIsHereToo(
    const std::source_location& location /* =
        std::source_location::current() */ ) const {
	report::debugMessage(
	    fmt::format("FO#{}: Waiting on everyvone to join me", getID()), {},
	    location);
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

void FrontOfficer::waitFor_publishAgentsAABBs() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

void FrontOfficer::waitForAllFOs(
    const std::source_location& location
    /* = std::source_location::current() */) const {
	report::debugMessage(
	    fmt::format("FO#{}: Waiting on FOs to join me", getID()), {}, location);
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
	std::vector<std::string> to_register;
	for (int fo = 1; fo <= int(got.size()); ++fo)
		for (const auto& [id, type_id, geom_version, AABB] : got[fo - 1]) {
			AABBs.emplace_back(AABB, id, type_id);
			agentsAndBroadcastGeomVersions[id] = geom_version;
			registerThatThisAgentIsAtThisFO(id, fo);

			if (!agentsTypesDictionary.isInDictionary(type_id)) {
				report::debugMessage(
				    fmt::format("FO#{} request type name of id: {} from FO#{}",
				                getID(), type_id, fo));
				MPIw::Send_one(impl.Async_request_comm, -1, fo,
				               async_tags::request_type_string);
				MPIw::Send_one(impl.Async_response_comm, type_id, fo,
				               async_tags::request_type_string);
				auto type = MPIw::Recv<char>(impl.Async_response_comm, fo,
				                             async_tags::send_type_string)
				                .data;
				to_register.emplace_back(type.begin(), type.end());
			}
		}

	waitForAllFOs();

	// register types now, to avoid race-condition with async thread
	for (const std::string& name : to_register)
		agentsTypesDictionary.registerThisString(name);
}

std::unique_ptr<ShadowAgent>
FrontOfficer::request_ShadowAgentCopy(const int agentID,
                                      const int FOsID) const {
	ImplementationData& impl = get_data(implementationData);
	// Send request
	MPIw::Send_one(impl.Async_request_comm, agentID, FOsID,
	               async_tags::request_shadow_agent);

	// Recieve data
	auto bytes = MPIw::Recv<std::byte>(impl.Async_response_comm, FOsID,
	                                   async_tags::send_shadow_agent)
	                 .data;

	return deserializeAgent(bytes);
}

// Not used, synchronization is provided by rendering frame in
// respond_renderNextFrame
void FrontOfficer::waitFor_renderNextFrame() const {}