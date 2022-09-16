#include "GrpcDisplayUnit.hpp"
#include "../util/report.hpp"

namespace mbv = mastodon_blender_view;

void print_status_err(const grpc::Status& status) {
	assert(!status.ok());

	report::error(fmt::format(
	    "GRPC failed with code: {}\nMessage: {}\nDetails: {}",
	    status.error_code(), status.error_message(), status.error_details()));
}

void GrpcDisplayUnit::DrawPoint(const int ID,
                                const Vector3d<float>& pos,
                                const float /* radius = 1.0f */,
                                const int /* color = 0 */) /* override */ {

	auto& point = _requestCache[ID];
	for (std::size_t i = 0; i < 3; ++i)
		point.coords.push_back(pos[i]);

	assert(point.timepoints.empty() || point.timepoints.back() < _timepoint);
	point.timepoints.push_back(_timepoint);
};

void GrpcDisplayUnit::DrawLine(const int /* ID */,
                               const Vector3d<float>& /* posA */,
                               const Vector3d<float>& /* posB */,
                               const int /* color = 0*/) /* override */ {}

void GrpcDisplayUnit::DrawVector(const int /* ID */,
                                 const Vector3d<float>& /* pos */,
                                 const Vector3d<float>& /* vector */,
                                 const int /* color = 0 */) /* override */ {}

void GrpcDisplayUnit::DrawTriangle(const int /* ID */,
                                   const Vector3d<float>& /* posA */,
                                   const Vector3d<float>& /* posB */,
                                   const Vector3d<float>& /* posC */,
                                   const int /* color = 0 */) /* override */ {}

void GrpcDisplayUnit::Tick(const std::string&) /* override */ { ++_timepoint; }

GrpcDisplayUnit::~GrpcDisplayUnit() /* override */
{
	Send();
}

void GrpcDisplayUnit::Send() {
	for (const auto& [ID, point] : _requestCache) {
		mbv::AddMovingSpotRequest req;
		req.set_id(std::to_string(ID));

		for (auto v : point.coords)
			req.add_coordinates(v);

		for (auto v : point.timepoints)
			req.add_timepoints(v);
		grpc::ClientContext context;
		mbv::Empty empty;

		grpc::Status status = _stub->addMovingSpot(&context, req, &empty);
		if (!status.ok())
			print_status_err(status);
	}
}