#pragma once
#include <grpcpp/grpcpp.h>

#include <chrono>
#include <fmt/chrono.h>
#include "../config.hpp"
#include "../generated/buckets_with_graphics.grpc.pb.h"
#include "../generated/buckets_with_graphics.pb.h"
#include "DisplayUnit.hpp"

namespace proto = transfers_graphics_protocol;
using perAgentBatch_dataType = proto::BatchOfGraphics;

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
	void Tick(const std::string& tick_msg) override;


	// ---------------- internal routines ----------------
	GrpcDisplayUnit(const std::string& clientName);
	~GrpcDisplayUnit() override;

	void setClientName(const std::string& new_name) {
		clientName = new_name;
		introduceClientToTheServer();
	}
	void introduceClientToTheServer();

	/** Unified signature string of multiple Blender writers,
	    e.g., when used in a "distributed configuration". */
	static std::string createFOnameOfThisId(const int id) {
		return fmt::format("EmbryoGen FO #{}", id);
	}

	std::string collectionName { fmt::format("{:%X %m-%d-%Y}", std::chrono::system_clock::now()) };
	std::string normalContentDataNamePrefix { "agent #" };
	std::string debugContentDataNamePrefix { "debug of #" };

	/**
	 * A time coordinate that is used with every xyz-positioned element
	 * that is to be displayed via this DisplayUnit, the time is automatically
	 * incremented with every Tick() (see also doTickIncrementsCurrentTimepoint
	 * attrib) and normally one does not need to adjust it...
	 * However, it is made available for corner use-cases.
	 */
	uint64_t currentTimepoint { 0 };

	/**
	 * If one wants to keep the display timepoint fixed, disable this flag
	 * (or keep resetting the currentTimepoint attribute...).
	 */
	bool doTickIncrementsCurrentTimepoint { true };

	/**
	 * When sending another batch of agent data/graphics, this flag indicates
	 * that older/previous data should be kept... in this way, the history of
	 * geometries is being built. If set to false, it is perhaps advisible to
	 * keep resetting the currentTimepoint to make the content displayed always
	 * at the same time slot in the DisplayUnit.
	 * Note that sending of the data/graphics happens in sendCurrentBatches(),
	 * which where this flag is only considered. That said, for example, one
	 * cannot be using different regime for the displaying of different agents
	 */
	bool doAppendInsteadOfReplace { true };

	void startKeepingGraphicsHistory() {
		doAppendInsteadOfReplace = true;
		doTickIncrementsCurrentTimepoint = true;
	}
	void startShowingOnlyCurrentGraphics() {
		doAppendInsteadOfReplace = false;
		doTickIncrementsCurrentTimepoint = false;
	}

  private:
	std::unique_ptr<proto::ClientToServer::Stub> _stub;
	std::string clientName;

	std::map<int, perAgentBatch_dataType> currentBatches_normalContent;
	std::map<int, perAgentBatch_dataType> currentBatches_debugContent;

	void resetCurrentBatches();
	perAgentBatch_dataType& getOrCreateBatchForId(
	        const int agent_id, std::map<int, perAgentBatch_dataType>& map,
	        const std::string& designationPrefix);
	perAgentBatch_dataType& getCurrentNormalBatchForId(const int agent_id);
	perAgentBatch_dataType& getCurrentDebugBatchForId(const int agent_id);
	void sendCurrentBatches();

	constexpr static float DEFAULT_LINE_RADIUS = 1.0f;
	constexpr static float DEFAULT_VECTOR_RADIUS = 1.0f;
};
