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
	MPIw::Comm_raii Async_request_comm;
	MPIw::Comm_raii Async_response_comm;
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
		MPI_Comm_dup(MPI_COMM_WORLD, &impl.Async_request_comm);
		MPI_Comm_dup(MPI_COMM_WORLD, &impl.Async_response_comm);
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
		init3();
	} else {
		impl.FO->init();
	}
}

void Director::execute() {
	ImplementationData& impl = get_data(implementationData);
	if (is_main_director(implementationData))
		_execute();
	else
		impl.FO->execute();
}

int Director::getFOsCount() const {
	return MPIw::Comm_size(MPI_COMM_WORLD) - 1;
}

// not used ... FOs know what to do
void Director::prepareForUpdateAndPublishAgents() const {}

// not used ... FOs know what to do
void Director::postprocessAfterUpdateAndPublishAgents() const {}

void Director::waitHereUntilEveryoneIsHereToo(
    const std::source_location& location
    /* = std::source_location::current() */) const {
	report::debugMessage("Director: Waiting on everyvone to join me", {},
	                     location);
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
	return MPIw::Gatherv_recv(impl.Dir_comm,
	                          std::vector<std::pair<int, bool>>{});
}
std::vector<std::vector<int>> Director::request_closedAgents() const {
	ImplementationData& impl = get_data(implementationData);
	return MPIw::Gatherv_recv(impl.Dir_comm, std::vector<int>{});
}
std::vector<std::vector<std::pair<int, int>>>
Director::request_parentalLinksUpdates() const {
	ImplementationData& impl = get_data(implementationData);
	return MPIw::Gatherv_recv(impl.Dir_comm,
	                          std::vector<std::pair<int, int>>{});
}

void Director::notify_setDetailedDrawingMode(int FOsID,
                                             int agentID,
                                             bool state) const {
	assert(FOsID == agents.at(agentID));
	ImplementationData& impl = get_data(implementationData);
	MPIw::Send_one(impl.Async_request_comm, agentID, FOsID,
	               async_tags::set_detailed_drawing);
	MPIw::Send_one(impl.Async_request_comm, state, FOsID,
	               async_tags::set_detailed_drawing);
}

void Director::notify_setDetailedReportingMode(int FOsID,
                                               int agentID,
                                               bool state) const {
	assert(FOsID == agents.at(agentID));
	ImplementationData& impl = get_data(implementationData);
	MPIw::Send_one(impl.Async_request_comm, agentID, FOsID,
	               async_tags::set_detailed_reporting);
	MPIw::Send_one(impl.Async_request_comm, state, FOsID,
	               async_tags::set_detailed_reporting);
}

void Director::request_renderNextFrame() const {

	SceneControls& sc = *scenario->params;
	ImplementationData& impl = get_data(implementationData);
	// MPI will not work with std::vector<bool>
	std::vector<char> to_render{sc.isProducingOutput(sc.imgMask),
	                            sc.isProducingOutput(sc.imgPhantom),
	                            sc.isProducingOutput(sc.imgOptics)};
	MPIw::Bcast_send(impl.Dir_comm, to_render);

	std::vector<std::remove_pointer_t<decltype(sc.imgMask.GetFirstVoxelAddr())>>
	    mask(sc.imgMask.GetImageSize());

	std::vector<
	    std::remove_pointer_t<decltype(sc.imgPhantom.GetFirstVoxelAddr())>>
	    phantom(sc.imgPhantom.GetImageSize());

	std::vector<
	    std::remove_pointer_t<decltype(sc.imgOptics.GetFirstVoxelAddr())>>
	    optics(sc.imgOptics.GetImageSize());

	auto got_mask = MPIw::Reduce_recv(impl.Dir_comm, mask, MPI_MAX);
	auto got_phantom = MPIw::Reduce_recv(impl.Dir_comm, phantom, MPI_MAX);
	auto got_optics = MPIw::Reduce_recv(impl.Dir_comm, optics, MPI_MAX);

	std::ranges::copy(got_mask, sc.imgMask.begin());
	std::ranges::copy(got_phantom, sc.imgPhantom.begin());
	std::ranges::copy(got_optics, sc.imgOptics.begin());
}

/* Optimized version of recieving images ... but yet still slower :D
void Director::request_renderNextFrame() const {

    SceneControls& sc = *scenario->params;
    ImplementationData& impl = get_data(implementationData);
    // MPI will not work with std::vector<bool>
    std::vector<char> to_render{sc.isProducingOutput(sc.imgMask),
                                sc.isProducingOutput(sc.imgPhantom),
                                sc.isProducingOutput(sc.imgOptics)};
    MPIw::Bcast_send(impl.Dir_comm, to_render);

    auto get_image = [&](auto& img, auto bin_op) {
        using value_type =
            std::remove_pointer_t<decltype(img.GetFirstVoxelAddr())>;

        auto offsets =
            MPIw::Gather_recv(impl.Dir_comm, std::array<std::size_t, 6>{0u});

        auto data =
            MPIw::Gatherv_recv(impl.Dir_comm, std::array<value_type, 0>{});

        for (std::size_t fo = 1; fo < data.size(); ++fo) {
            i3d::Vector3d<std::size_t> min_point;
            i3d::Vector3d<std::size_t> slice_size;
            for (std::size_t i = 0; i < 3; ++i) {
                min_point[i] = offsets[fo * 6 + i];
                slice_size[i] = offsets[fo * 6 + 3 + i];
            }

            assert(data[fo].size() ==
                   slice_size.x * slice_size.y * slice_size.z);

            std::size_t i = 0;
            for (std::size_t x = 0; x < slice_size.x; ++x)
                for (std::size_t y = 0; y < slice_size.y; ++y)
                    for (std::size_t z = 0; z < slice_size.z; ++z) {
                        i3d::Vector3d<std::size_t> coords =
                            min_point + i3d::Vector3d{x, y, z};
                        auto val = img.GetVoxel(coords);
                        img.SetVoxel(coords, bin_op(val, data[fo][i++]));
                    }
        }
    };

    auto max = [](auto a, auto b) { return std::max(a, b); };

    get_image(sc.imgMask, max);
    get_image(sc.imgPhantom, max);
    get_image(sc.imgOptics, max);
}
*/

// Not used, synchronization is provided by rendering frame in
// request_renderNextFrame
void Director::waitFor_renderNextFrame() const {}

void Director::broadcast_setRenderingDebug(bool state) const {
	ImplementationData& impl = get_data(implementationData);
	// Only send/recv is allowed in async mode
	for (int fo = 1; fo <= getFOsCount(); ++fo)
		MPIw::Send_one(impl.Async_request_comm, int(state), fo,
		               async_tags::set_rendering_debug);
}

// not used ... FOs know what to do
void Director::broadcast_executeInternals() const {}

// not used ... FOs know what to do
void Director::broadcast_executeExternals() const {}

// not used ... FOs know what to do
void Director::broadcast_executeEndSub1() const {}

// not used ... FOs know what to do
void Director::broadcast_executeEndSub2() const {}