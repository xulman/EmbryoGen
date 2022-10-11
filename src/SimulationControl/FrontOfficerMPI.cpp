#include "../Agents/AbstractAgent.hpp"
#include "../FrontOfficer.hpp"
#include "MPI_common.hpp"
#include <limits>

namespace {
class ImplementationData {
  public:
	MPIw::Comm_raii FO_comm;
	MPIw::Comm_raii Dir_comm;

  private:
};

/*
ImplementationData& get_data(const std::shared_ptr<void>& obj) {
    return *static_cast<ImplementationData*>(obj.get());
}
*/

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

	std::cout << fmt::format("FO: my_rank: {}, world_size: {}", myPortion,
	                         allPortions)
	          << std::endl;
}

// TODO
void FrontOfficer::init() {}

// TODO
void FrontOfficer::execute() {}

// TODO
void FrontOfficer::waitHereUntilEveryoneIsHereToo() const {}

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