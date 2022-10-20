#include "../Director.hpp"
#include "MPI_common.hpp"
#include <memory>

namespace {
class ImplementationData {
  private:
	// This has to be called first to be constructed first and destructed last
	MPIManager _mpi;

  public:
	std::unique_ptr<FrontOfficer> FO = nullptr;
	MPIw::Comm_raii Dir_comm;
};

ImplementationData& get_data(const std::shared_ptr<void>& obj) {
	return *static_cast<ImplementationData*>(obj.get());
}

bool is_main_director(const std::shared_ptr<void>& obj) {
	return get_data(obj).FO == nullptr;
}
} // namespace

Director::Director(std::function<ScenarioUPTR()> scenarioFactory)
    : scenario(scenarioFactory()),
      shallWaitForUserPromptFlag(scenario->params->shallWaitForUserPromptFlag),
      implementationData(std::make_shared<ImplementationData>()) {

	int my_rank = MPIw::Comm_rank(MPI_COMM_WORLD);
	int world_size = MPIw::Comm_size(MPI_COMM_WORLD);

	if (world_size == 1)
		throw report::rtError("Running MPI version with 1 worker is not "
		                      "allowed, please run: mpirun ./embryogen ...");

	ImplementationData& impl = get_data(implementationData);

	// Im main director
	if (my_rank == 0) {
		scenario->declareDirektorContext();
		MPI_Comm_dup(MPI_COMM_WORLD, &impl.Dir_comm);
	} else { // forward to FO
		impl.FO = std::make_unique<FrontOfficer>(std::move(scenario), my_rank,
		                                         getFOsCount());
	}
}

void Director::init() {
	ImplementationData& impl = get_data(implementationData);
	if (is_main_director(implementationData)) {
		init1();
		init2();
		// init3();
	} else {
		impl.FO->init();
	}
}

void Director::execute() {}

int Director::getFOsCount() const {
	return MPIw::Comm_size(MPI_COMM_WORLD) - 1;
}

// not used ... FOs know what to do
void Director::prepareForUpdateAndPublishAgents() const {}

void Director::postprocessAfterUpdateAndPublishAgents() const {}

void Director::waitHereUntilEveryoneIsHereToo() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

// not used ... FOs know what to do
void Director::notify_publishAgentsAABBs() const {}
void Director::waitFor_publishAgentsAABBs() const {
	MPIw::Barrier(get_data(implementationData).Dir_comm);
}

std::vector<std::size_t> Director::request_CntsOfAABBs() const {
	ImplementationData& impl = get_data(implementationData);
	return MPIw::Gather_recv_one<std::size_t>(impl.Dir_comm, 0);
}

std::vector<std::vector<std::pair<int, bool>>>
Director::request_startedAgents() const {
	ImplementationData& impl = get_data(implementationData);
	return MPIw::Gatherv_recv<std::pair<int, bool>>(impl.Dir_comm, {});
}
std::vector<std::vector<int>> Director::request_closedAgents() const {
	ImplementationData& impl = get_data(implementationData);
	return MPIw::Gatherv_recv<int>(impl.Dir_comm, {});
}
std::vector<std::vector<std::pair<int, int>>>
Director::request_parentalLinksUpdates() const {
	ImplementationData& impl = get_data(implementationData);
	return MPIw::Gatherv_recv<std::pair<int, int>>(impl.Dir_comm, {});
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