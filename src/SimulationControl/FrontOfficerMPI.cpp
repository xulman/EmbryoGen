#include "../FrontOfficer.hpp"
#include "MPI_common.hpp"
#include <limits>

namespace {
class ImplementationData {
  public:
	MPIw::Comm_raii FO_comm;
	MPIw::Comm_raii Dir_comm;
};

ImplementationData& get_data(const std::shared_ptr<void>& obj) {
	return *static_cast<ImplementationData*>(obj.get());
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

	// Create FO comm
	MPIw::Group_raii FO_group;
	MPI_Comm_group(MPI_COMM_WORLD, &FO_group);
	MPI_Group_excl(FO_group, 1, &RANK_DIRECTOR, &FO_group);
	MPI_Comm_create_group(MPI_COMM_WORLD, FO_group, 0, &impl.FO_comm);
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

// TODO
void FrontOfficer::execute() {}

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

	// Sending AABB count to director
	MPIw::Gather_send_one(impl.Dir_comm, getSizeOfAABBsList(), RANK_DIRECTOR);

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
};

void FrontOfficer::waitHereUntilEveryoneIsHereToo() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

void FrontOfficer::waitFor_publishAgentsAABBs() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
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

// TODO
std::unique_ptr<ShadowAgent>
FrontOfficer::request_ShadowAgentCopy(const int /* agentID */,
                                      const int /* FOsID */) const {
	// this never happens here (as there is no other FO to talk to)
	return nullptr;
}

void FrontOfficer::waitFor_renderNextFrame() const {}