#include "../Director.hpp"
#include "MPI_common.hpp"
#include <memory>

namespace {
class ImplementationData {
  private:
	// Must be written first, to be created first and destroyed last
	MPIw::Init_raii _mpi{nullptr, nullptr};

  public:
	std::unique_ptr<FrontOfficer> FO = nullptr;
	MPIw::Comm_raii Dir_comm;
};

ImplementationData& get_data(const std::shared_ptr<void>& obj) {
	return *static_cast<ImplementationData*>(obj.get());
}
} // namespace

Director::Director(std::function<ScenarioUPTR()> scenarioFactory)
    : scenario(scenarioFactory()),
      shallWaitForUserPromptFlag(scenario->params->shallWaitForUserPromptFlag),
      implementationData(std::make_shared<ImplementationData>()) {

	int my_rank = MPIw::Comm_rank(MPI_COMM_WORLD);
	int world_size = MPIw::Comm_size(MPI_COMM_WORLD);

	// Im main director
	if (my_rank == 0) {
		scenario->declareDirektorContext();
	} else { // forward to FO
		get_data(implementationData).FO = std::make_unique<FrontOfficer>(
		    std::move(scenario), my_rank, world_size);
	}

	std::cout << fmt::format("DIR: my_rank: {}, world_size: {}", my_rank,
	                         world_size)
	          << std::endl;
}

void Director::init() {}

// BARIERR

void Director::execute() {}

int Director::getFOsCount() const { return MPIw::Comm_size(MPI_COMM_WORLD); }

void Director::prepareForUpdateAndPublishAgents() const {}

void Director::postprocessAfterUpdateAndPublishAgents() const {}

void Director::waitHereUntilEveryoneIsHereToo() const {}

void Director::notify_publishAgentsAABBs() const {}
void Director::waitFor_publishAgentsAABBs() const {}

std::vector<std::size_t> Director::request_CntsOfAABBs() const { return {}; }

std::vector<std::vector<std::pair<int, bool>>>
Director::request_startedAgents() const {
	return {};
}
std::vector<std::vector<int>> Director::request_closedAgents() const {
	return {};
}
std::vector<std::vector<std::pair<int, int>>>
Director::request_parentalLinksUpdates() const {
	return {};
}

void Director::notify_setDetailedDrawingMode(int /* FOsID */,
                                             int /* agentID */,
                                             bool /* state */) const {}

void Director::notify_setDetailedReportingMode(int /* FOsID */,
                                               int /* agentID */,
                                               bool /* state */) const {}

void Director::request_renderNextFrame() const {}

void Director::waitFor_renderNextFrame() const {}

void Director::broadcast_setRenderingDebug(bool /* setFlagToThis*/) const {}

void Director::broadcast_executeInternals() const {}

void Director::broadcast_executeExternals() const {}

void Director::broadcast_executeEndSub1() const {}

void Director::broadcast_executeEndSub2() const {}