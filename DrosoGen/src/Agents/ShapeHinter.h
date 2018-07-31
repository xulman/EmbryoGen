#ifndef SHAPEHINTER_H
#define SHAPEHINTER_H

#include <list>
#include "../util/report.h"
#include "AbstractAgent.h"
#include "../Geometries/MaskImg.h"

class ShapeHinter: public AbstractAgent
{
public:
	ShapeHinter(const int ID, const MaskImg& shape,
	            const float currTime, const float incrTime)
		: AbstractAgent(ID,geometryAlias,currTime,incrTime),
		  geometryAlias(shape),
		  futureGeometry(shape)
	{
		//update AABBs
		geometryAlias.Geometry::setAABB();
		futureGeometry.Geometry::setAABB();

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
	MaskImg geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry, even the same noOfSpheres */
	MaskImg futureGeometry;

	// ------------- externals geometry -------------
	//FOR DEBUG ONLY!
	/** limiting distance beyond which I consider no interaction possible
	    with other nuclei */
	float ignoreDistance = 85.f;

	//FOR DEBUG ONLY!
	/** locations of possible interaction with nearby nuclei */
	std::list<ProximityPair> proximityPairs;

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override
	{

		//increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces(void) override
	{
		//update my futureGeometry
	}

	void collectExtForces(void) override
	{
		//FOR DEBUG ONLY!
		//scheduler, please give me ShadowAgents that are not further than ignoreDistance
		//(and the distance is evaluated based on distances of AABBs)
		std::list<const ShadowAgent*> l;
		Officer->getNearbyAgents(this,ignoreDistance,l);

		DEBUG_REPORT("Hinter: Found " << l.size() << " nearby agents");

		//those on the list are ShadowAgents who are potentially close enough
		//to interact with me and these I need to inspect closely
		proximityPairs.clear();
		for (auto sa = l.begin(); sa != l.end(); ++sa)
			geometry.getDistance((*sa)->getGeometry(),proximityPairs);

		//now, postprocess the proximityPairs
		DEBUG_REPORT("Hinter: Found " << proximityPairs.size() << " proximity pairs");
	}

	void adjustGeometryByExtForces(void) override
	{
		//update my futureGeometry
	}

	//futureGeometry -> geometryAlias
	void updateGeometry(void) override
	{
		//since we're not changing ShadowAgent::geometry (and consequently
		//not this.geometryAlias), we don't need to update this.futureGeometry
	}

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override
	{
		//draw bounding box of the GradIN region of the MaskImg
		int ID = this->ID << 17;
		ID += futureGeometry.AABB.drawIt(ID,1, du);

		//draw bounding box of the complete MaskImg, as a debug element
		ID |= 1 << 16; //enable debug bit
		futureGeometry.AABB.drawBox(ID,4,
		  futureGeometry.getDistImgOff(),futureGeometry.getDistImgFarEnd(), du);

/*
		if (ID == 1 || ID == 3)
		{
			int cnt=1;
			for (auto& p : proximityPairs)
				//du.DrawVector((ID << 17) +cnt++, p.localPos, p.otherPos-p.localPos);
				du.DrawLine((ID << 17) +cnt++, p.localPos, p.otherPos);
		}
*/
	}
};
#endif
