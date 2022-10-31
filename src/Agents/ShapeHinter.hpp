#pragma once

#include "../Geometries/ScalarImg.hpp"
#include "../util/report.hpp"
#include "AbstractAgent.hpp"
#include <fmt/core.h>

class ShapeHinter : public AbstractAgent {
  public:
	/** the same (given) shape is kept during the simulation */
	ShapeHinter(const int ID,
	            const std::string& type,
	            const ScalarImg& shape,
	            const float currTime,
	            const float incrTime)
	    : AbstractAgent(
	          ID, type, std::make_unique<ScalarImg>(shape), currTime, incrTime),
	      geometryAlias(*dynamic_cast<ScalarImg*>(geometry.get())) {
		// update AABBs
		geometryAlias.Geometry::updateOwnAABB();

		report::debugMessage(
		    fmt::format("EmbryoShell with ID={} was just created", ID));
		report::debugMessage(fmt::format(
		    "AABB: {} -> {}", toString(geometryAlias.AABB.minCorner),
		    toString(geometryAlias.AABB.maxCorner)));
	}

	~ShapeHinter() override {
		report::debugMessage(
		    fmt::format("EmbryoShell with ID= {} was just deleted", ID));
	}

  private:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	ScalarImg& geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry */
	// ScalarImg futureGeometry;

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override {

		// increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces(void) override {
		// would update my futureGeometry
	}

	void collectExtForces(void) override {
		// hinter is not responding to its surrounding
	}

	void adjustGeometryByExtForces(void) override {
		// would update my futureGeometry
	}

	// futureGeometry -> geometryAlias
	void publishGeometry(void) override {
		// since we're not changing ShadowAgent::geometry (and consequently
		// not this.geometryAlias), we don't need to update this.futureGeometry
	}

	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
	void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override;
};