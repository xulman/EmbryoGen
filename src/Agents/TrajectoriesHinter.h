#ifndef AGENTS_TRAJECTORIESHINTER_H
#define AGENTS_TRAJECTORIESHINTER_H

#include "../util/report.h"
#include "AbstractAgent.h"
#include "../Geometries/VectorImg.h"
#include "../TrackRecord.h"

class TrajectoriesHinter: public AbstractAgent
{
public:
	template <class T>
	TrajectoriesHinter(const int ID, const std::string& type,
	                   const i3d::Image3d<T>& templateImg,
	                   const VectorImg::ChoosingPolicy policy,
	                   const float currTime, const float incrTime)
		: AbstractAgent(ID,type, geometryAlias, currTime,incrTime),
		  geometryAlias(templateImg,policy)
	{
		//update AABBs
		geometryAlias.Geometry::updateOwnAABB();
		geometryAlias.proxifyFF(ff);
		lastUpdatedTime = currTime-1;

		DEBUG_REPORT("EmbryoTracks with ID=" << ID << " was just created");
		DEBUG_REPORT("AABB: " << geometryAlias.AABB.minCorner << " -> " << geometryAlias.AABB.maxCorner);
	}

	~TrajectoriesHinter(void)
	{
		DEBUG_REPORT("EmbryoTraces with ID=" << ID << " was just deleted");
	}

	TrackRecords& talkToHinter()
	{
		return traHinter;
	}


private:
	// ------------- internals state -------------
	/** manager of the trajectories of all tracks */
	TrackRecords traHinter;

	/** binding FF between traHinter and geometryAlias */
	FlowField<float> ff;

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	VectorImg geometryAlias;
	float lastUpdatedTime;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry */
	//VectorImg futureGeometry;

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
		if (currTime > lastUpdatedTime)
		{
			DEBUG_REPORT(IDSIGN << "updating FF from " << currTime-incrTime << " to " << currTime);

			//update the geometryAlias according to the currTime
			traHinter.resetToFF(currTime-incrTime,currTime, ff, Vector3d<float>(5.0f));
			lastUpdatedTime = currTime;
		}
		else
		{
			DEBUG_REPORT(IDSIGN << "skipping update now");
		}
	}

	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
	//void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override;
};
#endif
