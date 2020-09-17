#ifndef AGENTS_NUCLEUS2pNSAGENT_H
#define AGENTS_NUCLEUS2pNSAGENT_H

#include <memory>
#include "NucleusAgent.h"

class Nucleus2pNSAgent: public NucleusAgent
{
public:
	Nucleus2pNSAgent(const int _ID, const std::string& _type,
	                 const Spheres& shape, const Vector3d<FLOAT>& _basalPlaneNormal,
	                 const float _currTime, const float _incrTime):
		NucleusAgent(_ID,_type,shape, _currTime,_incrTime),
		spheresLocalCoords(shape.noOfSpheres-2), basalPlaneNormal(_basalPlaneNormal),
		offs(new Vector3d<FLOAT>[shape.noOfSpheres-2]),
		overlaps(new FLOAT[shape.noOfSpheres])
	{
		if (shape.noOfSpheres < 2)
			throw ERROR_REPORT("Cannot construct Nucleus2pNSAgent with less than two sphere geometry.");

		//init centreDistance based on the initial geometry
		centreDistance = (futureGeometry.centres[1] - futureGeometry.centres[0]).len();

		//init local coords of the initial geometry
		defineSpheresLocalCoords();
	}


protected:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	/** the wanted distance between the first two spheres */
	FLOAT centreDistance;
	/** the wanted positions of centres (w.r.t. the local (inner) cell
	    coordinate system) of all but the first two spheres */
	std::vector<Vector3d<FLOAT>> spheresLocalCoords;

	/** the (given) second major axis to define the local (inner) cell
	    coordinate system, the vector is expected to be of unit length */
	Vector3d<FLOAT> basalPlaneNormal;

	/** determine the local (inner) coords of all but the first two spheres,
	    this can be used every time the 'spheresLocalCoords' should be updated
	    to the current shape in the 'futureGeometry', or when 'basalPlaneNormal'
	    is changed */
	void defineSpheresLocalCoords();


	/** this helper axis is a function of the 'futureGeometry' state, use updateAuxAxes() */
	Vector3d<FLOAT> centreAxis;
	/** this helper axis is a function (cross product) of the 'centreAxis' and 'basalPlaneNormal', use updateAuxAxes() */
	Vector3d<FLOAT> aux3rdAxis;

	/** updates and normalizes the local axes 'centreAxis' and 'aux3rdAxis' */
	void updateAuxAxes();


	/** calculates a difference from the expected position of centres of all but
	    the first two spheres from their current position, and stores it into 'offs' */
	void getCurrentOffVectorsForCentres();

	/** this is the raw array of Vector3ds to work with,
	    the array is filled with this.getCurrentOffVectorsForCentres() */
	std::unique_ptr< Vector3d<FLOAT>[] > offs;

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

protected:
	// ------------- rendering -------------
	//void drawForDebug(DisplayUnit& du) override;
};
#endif
