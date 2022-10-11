#include "../Agents/AbstractAgent.hpp"
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
      nextAvailAgentID(std::numeric_limits<int>::max() / allPortions *
                       (myPortion - 1)),
      implementationData(std::make_shared<ImplementationData>()) {
	scenario->declareFOcontext(myPortion);

	ImplementationData& impl = get_data(implementationData);
	MPI_Comm_dup(MPI_COMM_WORLD, &impl.Dir_comm);

	// Create FO comm
	MPIw::Group_raii FO_group;
	MPI_Comm_group(MPI_COMM_WORLD, &FO_group);
	MPI_Group_excl(FO_group, 1, &RANK_DIRECTOR, &FO_group);
	MPI_Comm_create_group(MPI_COMM_WORLD, FO_group, 0, &impl.FO_comm);

	std::cout << fmt::format(
	                 "FO: dir_rank: {}, dir_size: {}, fo_rank: {}, fo_size: {}",
	                 MPIw::Comm_rank(impl.Dir_comm),
	                 MPIw::Comm_size(impl.Dir_comm),
	                 MPIw::Comm_rank(impl.FO_comm),
	                 MPIw::Comm_size(impl.FO_comm))
	          << std::endl;
}

void FrontOfficer::init() {
	init1();
	init2();

	ImplementationData& impl = get_data(implementationData);
	prepareForUpdateAndPublishAgents();
	// Sending closed agents to Director
	MPIw::Gatherv_send(impl.Dir_comm, getClosedAgents(), RANK_DIRECTOR);
	// Sending started agents to Director
	MPIw::Gather_send(impl.Dir_comm, getStartedAgents(), RANK_DIRECTOR);

	waitHereUntilEveryoneIsHereToo();
}

// TODO
void FrontOfficer::execute() {}

// TODO
void FrontOfficer::waitHereUntilEveryoneIsHereToo() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

// TODO
void FrontOfficer::waitFor_publishAgentsAABBs() const {}

// TODO
void FrontOfficer::broadcast_AABBofAgents() {}

// TODO
std::unique_ptr<ShadowAgent>
FrontOfficer::request_ShadowAgentCopy(const int /* agentID */,
                                      const int /* FOsID */) const {
	// this never happens here (as there is no other FO to talk to)
	return nullptr;
}

void FrontOfficer::waitFor_renderNextFrame() const {}