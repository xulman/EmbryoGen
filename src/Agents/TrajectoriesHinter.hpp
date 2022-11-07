#pragma once

#include "../Geometries/VectorImg.hpp"
#include "../TrackRecord.hpp"
#include "../util/report.hpp"
#include "AbstractAgent.hpp"

class TrajectoriesHinter : public AbstractAgent {
  public:
	TrajectoriesHinter(const int ID,
	                   std::string type,
	                   const VectorImg& shape,
	                   const float currTime,
	                   const float incrTime)
	    : AbstractAgent(ID,
	                    std::move(type),
	                    std::make_unique<VectorImg>(shape),
	                    currTime,
	                    incrTime),
	      geometryAlias(dynamic_cast<VectorImg*>(geometry.get())) {
		// update AABBs
		geometryAlias->Geometry::updateOwnAABB();
		geometryAlias->proxifyFF(ff);
		lastUpdatedTime = currTime - 1;

		report::debugMessage(
		    fmt::format("EmbryoTracks with ID={} was just created", ID));
		report::debugMessage(fmt::format(
		    "AABB: {} -> {}", toString(geometryAlias->AABB.minCorner),
		    toString(geometryAlias->AABB.maxCorner)));
	}

	TrajectoriesHinter(const TrajectoriesHinter&) = delete;
	TrajectoriesHinter& operator=(const TrajectoriesHinter&) = delete;

	// Geometry alias does not update :(
	TrajectoriesHinter(TrajectoriesHinter&&) = delete;
	TrajectoriesHinter& operator=(TrajectoriesHinter&&) = delete;

	~TrajectoriesHinter() {
		report::debugMessage(
		    fmt::format("EmbryoTraces with ID={} was just deleted", ID));
	}

	TrackRecords& getRecords() { return traHinter; }

	/** --------- Serialization support --------- */
	virtual std::vector<std::byte> serialize() const override {
		std::vector<std::byte> out;

		auto sa = ShadowAgent::serialize();
		out += Serialization::toBytes(sa.size());
		out += sa;

		out += Serialization::toBytes(currTime);
		out += Serialization::toBytes(incrTime);

		return out;
	}
	virtual agent_class getAgentClass() const override {
		return agent_class::TrajectoriesHinter;
	}

	static std::unique_ptr<TrajectoriesHinter>
	deserialize(std::span<const std::byte> bytes) {
		auto sa_size = extract<std::size_t>(bytes);

		auto sa = ShadowAgent::deserialize(bytes.first(sa_size));
		bytes = bytes.last(bytes.size() - sa_size);

		auto currTime = extract<float>(bytes);
		auto incrTime = extract<float>(bytes);

		auto th = std::make_unique<TrajectoriesHinter>(
		    sa->getID(), sa->getAgentType(),
		    dynamic_cast<const VectorImg&>(sa->getGeometry()), currTime,
		    incrTime);
		--th->geometry->version;
		return th;
	}

  protected:
	// ------------- internals state -------------
	/** manager of the trajectories of all tracks */
	TrackRecords traHinter;

	/** binding FF between traHinter and geometryAlias */
	FlowField<Geometry::precision_t> ff;

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	VectorImg* geometryAlias;
	float lastUpdatedTime;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry */
	// VectorImg futureGeometry;

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override {
		// increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces() override {
		// would update my futureGeometry
	}

	void collectExtForces() override {
		// hinter is not responding to its surrounding
	}

	void adjustGeometryByExtForces() override {
		// would update my futureGeometry
	}

	// futureGeometry -> geometryAlias
	void publishGeometry() override {
		if (currTime > lastUpdatedTime) {
			report::debugMessage(fmt::format("{}updating FF from {} to {}",
			                                 getSignature(),
			                                 currTime - incrTime, currTime));

			// update the geometryAlias according to the currTime
			traHinter.resetToFF(currTime - incrTime, currTime, ff,
			                    Vector3d<float>(5.0f));
			lastUpdatedTime = currTime;
		} else {
			report::debugMessage(
			    fmt::format("{} skipping update now", getSignature()));
		}
	}

	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
	// void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override;
};
