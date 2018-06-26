#ifndef NUCLEUSAGENT_H
#define NUCLEUSAGENT_H

#include "AbstractAgent.h"
#include "CellCycle.h"
#include "../Geometries/Spheres.h"

class NucleusAgent: public AbstractAgent
{
public:
	NucleusAgent(const int _ID, const Spheres& _shape,
	             const float _currTime, const float _incrTime)
		: AbstractAgent(_ID,*(new Spheres(_shape))),
		  futureGeometry(_shape)
	{
		currTime = _currTime;
		incrTime = _incrTime;

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
	void advanceAndBuildIntForces(void)
	{
	}

	void adjustGeometryByIntForces(void)
	{
	}

	void collectExtForces(void)
	{
	}

	void adjustGeometryByExtForces(void)
	{
	}

	void updateGeometry(void)
	{
	}

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du)
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
