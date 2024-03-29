#ifndef AGENTS_NUCLEUSNSAGENT_H
#define AGENTS_NUCLEUSNSAGENT_H

#include "NucleusAgent.h"
#include "../Geometries/util/SpheresFunctions.h"

/**
 * In the NS model (this class) the mutual arrangement of spheres is...
 */
class NucleusNSAgent: public NucleusAgent
{
public:
	NucleusNSAgent(const int _ID, const std::string& _type,
	               const Spheres& shape,
	               const float _currTime, const float _incrTime):
		NucleusAgent(_ID,_type,shape, _currTime,_incrTime),
		distanceMatrix(shape.noOfSpheres)
	{
		if (shape.noOfSpheres < 2)
			throw ERROR_REPORT("Cannot construct NucleusNSAgent with less than two spheres geometry.");
		resetDistanceMatrix();
	}


protected:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	SpheresFunctions::SquareMatrix<G_FLOAT> distanceMatrix;

	void resetDistanceMatrix();
	void printDistanceMatrix();

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

#ifdef DEBUG
	/** indices of spheres that gave rise to s2s force on some other sphere */
	std::vector<int> forces_s2sInducers;
#endif

protected:
	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override;
	void drawForDebug(DisplayUnit& du) override;
};
#endif
