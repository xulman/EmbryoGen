#ifndef NUCLEUSAGENT_H
#define NUCLEUSAGENT_H

#include "../util/report.h"
#include "AbstractAgent.h"
#include "CellCycle.h"
#include "../Geometries/Spheres.h"

class NucleusAgent: public AbstractAgent
{
public:
	NucleusAgent(const int ID, const Spheres& shape,
	             const float currTime, const float incrTime)
		: AbstractAgent(ID,*(new Spheres(shape)),currTime,incrTime),
		  futureGeometry(shape)
	{
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
	Spheres futureGeometry;

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override
	{

		//increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces(void) override
	{
	}

	void collectExtForces(void) override
	{
		//scheduler, please give me ShadowAgents that are not further than myself-given delta
		//(and the distance is evaluated based on distances of AABBs)
		//
		// [scheduler will consider also ShadowAgents from neighboring tiles]

		//those on the list are ShadowAgents who are potentially close enough to interact with me
		//and these I need to inspect closely
		//
		// [there will be a scheduler function for the detailed distance calculation to
		//  be able to cache the result... since real AbstractAgents from the same tile
		//  could ask for the same calculation, however ShadowAgents (that are AbstractAgents
		//  in some other tile) will never ask and so calculations to these need not be cached]

		std::list<const ShadowAgent*> l;
		Officer->getNearbyAgents(this,170.f,l);

		//now process the list

		//ONLY FOR DEBUG !!!!
		if (l.size() > 0)
		{
			DEBUG_REPORT("ID " << ID << ": These are nearby:");
			for (auto sa = l.begin(); sa != l.end(); sa++)
			{
				//we know that geometries are Spheres... for now!
				const Spheres* s = (Spheres*)&((*sa)->getGeometry());
				DEBUG_REPORT(s->getCentres()[0]);
			}
		}
	}

	void adjustGeometryByExtForces(void) override
	{
	}

	void updateGeometry(void) override
	{

		//update AABB
		geometry.setAABB();
	}

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override
	{
		int i=0;
		while (i < futureGeometry.noOfSpheres && futureGeometry.radii[i] > 0.f)
		{
			du.DrawPoint(ID,futureGeometry.centres[i],futureGeometry.radii[i],(curPhase < 3? 2:3));
			++i;
		}
	}
};
#endif
