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
	void advanceAndBuildIntForces(void) override
	{
	}

	void adjustGeometryByIntForces(void) override
	{
	}

	void collectExtForces(void) override
	{
	}

	void adjustGeometryByExtForces(void) override
	{
	}

	void updateGeometry(void) override
	{
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
