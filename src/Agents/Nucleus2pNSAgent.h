#ifndef AGENTS_NUCLEUS2pNSAGENT_H
#define AGENTS_NUCLEUS2pNSAGENT_H

#include <memory>
#include "NucleusAgent.h"

/**
 * In the 2pNS model (this class) the mutual arrangement of spheres is represented
 * relatively w.r.t. the axis between the first two spheres (current direction is
 * cached in 'centreAxis') and the firm 'basalPlaneNormal' vector. So, the actual
 * position of the first two spheres is given only with the distance between the
 * two ('centreDistance'). The remaining spheres are projected to real world coords
 * using the firm local coords ('spheresLocalCoords') and the current state of
 * the axes ('centreAxis', 'basalPlaneNormal', 'aux3rdAxis'). Thus, the expected
 * position of the remaining spheres depends on the current position of the first
 * two spheres.
 *
 * Whenever a sphere is detected to be off its expected position, a 's2s' force F
 * is created whose orientation is towards the expected position and magnitude is
 * driven by the size of the current displacement. Furthermore, opposite, equally
 * strong, force is "distributed" across spheres that are in touch with this one.
 * That said, a sum of these forces amounts to -F. The weights at which the forces
 * are distributed are proportional to the overlap of the sphere in question and
 * the offset sphere (the one with the full force F).
 */
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


	/** for all but the first two spheres, this method calculates a difference
	    from the current position of spheres centres to the expected position,
	    and stores it into 'offs' */
	void getCurrentOffVectorsForCentres();

	/** this is the raw array of Vector3ds to work with,
	    the array is filled with this.getCurrentOffVectorsForCentres() */
	std::unique_ptr< Vector3d<FLOAT>[] > offs;

	/** an aux array for distributeCounterForcesAmongTouchingSpheres() that is
	    living here to shield it from re-allocating with every method call */
	std::unique_ptr< FLOAT[] > overlaps;

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	/** Here, the 's2s - sphere2sphere' forces are created on demand, acting on
	    individual (centres of) spheres based on the difference of the centres from
	    their expected positions. In the end, the NucleusAgent::advanceAndBuildIntForces()
	    is called... to actually determine the final force per sphere and
	    implement geometry changes. */
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

	/** given a force acting on a certain sphere, it creates scaled forces, all
	    parallel and opposite to the given one, at spheres that are overlapping
	    with this one, scaling is proportional to the overlap */
	void distributeCounterForcesAmongTouchingSpheres(const ForceVector3d<FLOAT>& counterForce);

protected:
	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override;
	void drawForDebug(DisplayUnit& du) override;
};
#endif
