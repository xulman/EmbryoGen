#ifndef AGENTS_SHAPEHINTER_H
#define AGENTS_SHAPEHINTER_H

#include "../util/report.hpp"
#include "AbstractAgent.hpp"
#include "../Geometries/ScalarImg.hpp"

class ShapeHinter: public AbstractAgent
{
public:
	/** the same (given) shape is kept during the simulation */
	ShapeHinter(const int ID, const std::string& type,
	            const ScalarImg& shape,
	            const float currTime, const float incrTime)
		: AbstractAgent(ID,type, geometryAlias, currTime,incrTime),
		  geometryAlias(shape)
	{
		//update AABBs
		geometryAlias.Geometry::updateOwnAABB();

		DEBUG_REPORT("EmbryoShell with ID=" << ID << " was just created");
		DEBUG_REPORT("AABB: " << geometryAlias.AABB.minCorner << " -> " << geometryAlias.AABB.maxCorner);
	}

	~ShapeHinter(void)
	{
		DEBUG_REPORT("EmbryoShell with ID=" << ID << " was just deleted");
	}


private:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	ScalarImg geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry */
	//ScalarImg futureGeometry;

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override
	{

		//increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces(void) override
	{
		//would update my futureGeometry
	}

	void collectExtForces(void) override
	{
		//hinter is not responding to its surrounding
	}

	void adjustGeometryByExtForces(void) override
	{
		//would update my futureGeometry
	}

	//futureGeometry -> geometryAlias
	void publishGeometry(void) override
	{
		//since we're not changing ShadowAgent::geometry (and consequently
		//not this.geometryAlias), we don't need to update this.futureGeometry
	}

	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
	void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override;
};
#endif
