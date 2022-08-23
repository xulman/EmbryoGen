#pragma once
#include <grpcpp/grpcpp.h>

#include "../config.hpp"
#include "../proto/protocol.grpc.pb.h"
#include "../proto/protocol.pb.h"
#include "DisplayUnit.hpp"

class GrpcDisplayUnit : public DisplayUnit {
  public:
	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) override;

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override;

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override;

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override;

	/** Used as an indicator of new timepoint */
	void Tick(const std::string&) override;

  private:
	int _timepoint = 0;

	const std::string _serverURL =
	    fmt::format("{}:{}", config::grpc_serverAddress, config::grpc_port);

	std::unique_ptr<mastodon_blender_view::ViewService::Stub> _stub =
	    mastodon_blender_view::ViewService::NewStub(grpc::CreateChannel(
	        _serverURL, grpc::InsecureChannelCredentials()));
};