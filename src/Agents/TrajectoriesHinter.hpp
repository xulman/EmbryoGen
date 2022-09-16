#ifndef AGENTS_TRAJECTORIESHINTER_H
#define AGENTS_TRAJECTORIESHINTER_H

#include "../util/report.hpp"
#include "AbstractAgent.hpp"
#include "../Geometries/VectorImg.hpp"
#include "../TrackRecord.hpp"

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

report::debugMessage(fmt::format("EmbryoTracks with ID={} was just created" , ID));
report::debugMessage(fmt::format("AABB: {} -> {}" , toString(geometryAlias.AABB.minCorner), toString(geometryAlias.AABB.maxCorner)));
	}

	~TrajectoriesHinter(void)
	{
report::debugMessage(fmt::format("EmbryoTraces with ID={} was just deleted" , ID));
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
report::debugMessage(fmt::format("{}updating FF from {} to {}" , getSignature(), currTime-incrTime, currTime));

			//update the geometryAlias according to the currTime
			traHinter.resetToFF(currTime-incrTime,currTime, ff, Vector3d<float>(5.0f));
			lastUpdatedTime = currTime;
		}
		else
		{
report::debugMessage(fmt::format("{} skipping update now" , getSignature()));
		}
	}

	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
	//void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override;
};
#endif
