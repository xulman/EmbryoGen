#pragma once

#include "../Geometries/ScalarImg.hpp"
#include "../util/report.hpp"
#include "AbstractAgent.hpp"
#include <fmt/core.h>

class ShapeHinter : public AbstractAgent {
  public:
	/** the same (given) shape is kept during the simulation */
	ShapeHinter(const int ID,
	            std::string type,
	            const ScalarImg& shape,
	            const float currTime,
	            const float incrTime)
	    : AbstractAgent(ID,
	                    std::move(type),
	                    std::make_unique<ScalarImg>(shape),
	                    currTime,
	                    incrTime),
	      geometryAlias(dynamic_cast<ScalarImg*>(geometry.get())) {
		// update AABBs
		geometryAlias->Geometry::updateOwnAABB();

		report::debugMessage(
		    fmt::format("EmbryoShell with ID={} was just created", ID));
		report::debugMessage(fmt::format(
		    "AABB: {} -> {}", toString(geometryAlias->AABB.minCorner),
		    toString(geometryAlias->AABB.maxCorner)));
	}

	~ShapeHinter() override {
		report::debugMessage(
		    fmt::format("EmbryoShell with ID= {} was just deleted", ID));
	}

	ShapeHinter(const ShapeHinter&) = delete;
	ShapeHinter& operator=(const ShapeHinter&) = delete;

	// Geometry alias does not update :(
	ShapeHinter(ShapeHinter&&) = delete;
	ShapeHinter& operator=(ShapeHinter&&) = delete;

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
		return agent_class::ShapeHinter;
	}

	static std::unique_ptr<ShapeHinter>
	deserialize(std::span<const std::byte> bytes) {
		auto sa_size = extract<std::size_t>(bytes);

		auto sa = ShadowAgent::deserialize(bytes.first(sa_size));
		bytes = bytes.last(bytes.size() - sa_size);

		auto currTime = extract<float>(bytes);
		auto incrTime = extract<float>(bytes);

		auto sh = std::make_unique<ShapeHinter>(
		    sa->getID(), sa->getAgentType(),
		    dynamic_cast<const ScalarImg&>(sa->getGeometry()), currTime,
		    incrTime);
		--sh->geometry->version;
		return sh;
	}

  protected:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	ScalarImg* geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry */
	// ScalarImg futureGeometry;

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
		// since we're not changing ShadowAgent::geometry (and consequently
		// not this.geometryAlias), we don't need to update this.futureGeometry
	}

	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
	void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override;
};