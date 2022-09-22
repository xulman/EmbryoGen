#pragma once

#include "NucleusAgent.hpp"

class Nucleus4SAgent : public NucleusAgent {
  public:
	Nucleus4SAgent(const int _ID,
	               const std::string& _type,
	               const Spheres& shape,
	               const float _currTime,
	               const float _incrTime)
	    : NucleusAgent(_ID, _type, shape, _currTime, _incrTime) {
		if (shape.getNoOfSpheres() != 4)
			throw report::rtError(
			    "Cannot construct Nucleus4SAgent on non-four sphere geometry.");

		// init centreDistances based on the initial geometry
		centreDistance[0] =
		    float((geometryAlias.centres[1] - geometryAlias.centres[0]).len());
		centreDistance[1] =
		    float((geometryAlias.centres[2] - geometryAlias.centres[1]).len());
		centreDistance[2] =
		    float((geometryAlias.centres[3] - geometryAlias.centres[2]).len());
	}

  protected:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	/** canonical distance between the four cell centres that this agent
	    should maintain during geometry changes during the simulation */
	float centreDistance[3];

	/** assuming that the four spheres (that make up this nucleus) should lay
	    on a line with their centres apart as given in centreDistances[], this
	    method computes vector for every sphere centre that show how much
	    it is displaced from its expected position given that the assumed line
	    goes through the inner two spheres */
	void getCurrentOffVectorsForCentres(Vector3d<float> offs[4]);

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

  protected:
	// ------------- rendering -------------
	void drawForDebug(DisplayUnit& du) override;
};