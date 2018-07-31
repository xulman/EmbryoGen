#ifndef NUCLEUSAGENT_H
#define NUCLEUSAGENT_H

#include <list>
#include "../util/report.h"
#include "AbstractAgent.h"
#include "CellCycle.h"
#include "../Geometries/Spheres.h"

class NucleusAgent: public AbstractAgent
{
public:
	NucleusAgent(const int ID, const Spheres& shape,
	             const float currTime, const float incrTime)
		: AbstractAgent(ID,geometryAlias,currTime,incrTime),
		  geometryAlias(shape),
		  futureGeometry(shape)
	{
		//update AABBs
		geometryAlias.Geometry::setAABB();
		futureGeometry.Geometry::setAABB();

		curPhase = G1Phase;

		DEBUG_REPORT("Nucleus with ID=" << ID << " was just created");
	}

	~NucleusAgent(void)
	{
		DEBUG_REPORT("Nucleus with ID=" << ID << " was just deleted");
	}


private:
	// ------------- internals state -------------
	CellCycleParams cellCycle;

	/** currently exhibited cell phase */
	ListOfPhases curPhase;

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	Spheres geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry, even the same noOfSpheres */
	Spheres futureGeometry;

	// ------------- externals geometry -------------
	/** limiting distance beyond which I consider no interaction possible
	    with other nuclei */
	float ignoreDistance = 20.f;

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
		//scheduler, please give me ShadowAgents that are not further than ignoreDistance
		//(and the distance is evaluated based on distances of AABBs)
		std::list<const ShadowAgent*> l;
		Officer->getNearbyAgents(this,ignoreDistance,l);

		DEBUG_REPORT("ID " << ID << ": Found " << l.size() << " nearby agents");

		//those on the list are ShadowAgents who are potentially close enough
		//to interact with me and these I need to inspect closely
		proximityPairs.clear();
		for (auto sa = l.begin(); sa != l.end(); ++sa)
		{
			DEBUG_REPORT("BBtype: " << (*sa)->getGeometry().shapeForm);
				geometry.getDistance((*sa)->getGeometry(),proximityPairs);
		}

		//now, postprocess the proximityPairs
		DEBUG_REPORT("ID " << ID << ": Found " << proximityPairs.size() << " proximity pairs");
	}

	void adjustGeometryByExtForces(void) override
	{
		//update my futureGeometry
	}

	void updateGeometry(void) override
	{
		//promote my futureGeometry to my geometry, which happens
		//to be overlaid/mapped-over with geometryAlias (see the constructor)
		for (int i=0; i < geometryAlias.noOfSpheres; ++i)
		{
			geometryAlias.centres[i] = futureGeometry.centres[i];
			geometryAlias.radii[i]   = futureGeometry.radii[i];
		}

		//update AABB
		geometryAlias.Geometry::setAABB();
	}

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override
	{
		const int color = curPhase < 3? 2:3;
		int ID = this->ID << 17;

		//draw spheres
		int i=0;
		while (i < futureGeometry.noOfSpheres && futureGeometry.radii[i] > 0.f)
		{
			du.DrawPoint(ID++,futureGeometry.centres[i],futureGeometry.radii[i],color);
			++i;
		}

		int gID = this->ID << 12;
		//draw (not debug) vectors
		//if (this->ID == 1 || this->ID == 3)
		{
			for (auto& p : proximityPairs)
			{
				if (p.distance < 990000)
				{
					//normal distance measured
					du.DrawVector(ID, p.localPos, p.otherPos-p.localPos);
					//du.DrawLine(ID, p.localPos, p.otherPos);
				}
				else
				{
					//debugging
					int color = (int)p.distance -998893; // in { 3,4,5,6 }
					float radius = (int)p.distance == 998897 ? 1.1f : 1.0f;
					      radius = (int)p.distance == 998896 ? 0.2f : radius;
					radius *= 1.43f;
					int nID = (int)p.distance <= 998897 ? ID | (1 << 16) : gID++;
					if ((int)p.distance != 998899)
					du.DrawPoint(nID, p.localPos, radius,color);
				}
				++ID;
			}
		}

		//draw debug bounding box
		ID |= 1 << 16; //enable debug bit
		futureGeometry.AABB.drawIt(ID,color,du);
	}
};
#endif
