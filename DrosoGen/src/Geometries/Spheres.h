#ifndef SPHERES_H
#define SPHERES_H

#include "Geometry.h"

/**
 * Collection of Spheres::noOfSpheres spheres, the represented shape/geometry
 * is given as the union of these spheres.
 *
 * Author: Vladimir Ulman, 2018
 */
class Spheres: public Geometry
{
private:
	///length of the this.centres and this.radii arrays
	const int noOfSpheres;

	///list of centres of the spheres
	Vector3d<FLOAT>* const centres;

	///list of radii of the spheres
	FLOAT* const radii;

public:
	///empty shape constructor
	Spheres(const int _noOfSpheres)
		: Geometry(ListOfShapes::Spheres),
		  noOfSpheres(_noOfSpheres),
		  centres(new Vector3d<FLOAT>[noOfSpheres]),
		  radii(new FLOAT[noOfSpheres])
	{
		for (int i=0; i < noOfSpheres; ++i) radii[i] = 0.0;
	}

	///copy constructor
	Spheres(const Spheres& s)
		: Geometry(ListOfShapes::Spheres),
		  noOfSpheres(s.getNoOfSpheres()),
		  centres(new Vector3d<FLOAT>[noOfSpheres]),
		  radii(new FLOAT[noOfSpheres])
	{
		const Vector3d<FLOAT>* sCentres = s.getCentres();
		const FLOAT* sRadii = s.getRadii();

		for (int i=0; i < noOfSpheres; ++i)
		{
			centres[i] = sCentres[i];
			radii[i]   = sRadii[i];
		}
	}


	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Geometry& otherGeometry) const
	{
		switch (otherGeometry.shapeStyle)
		{
		case ListOfShapes::Spheres:
			//TODO
			REPORT("this.Spheres vs Spheres is not implemented yet!");
			break;
		case ListOfShapes::Mesh:
			return otherGeometry.getDistance(*this);
			break;
		case ListOfShapes::MaskImg:
			//TODO: attempt to render spheres into the mask image and look for collision
			REPORT("this.Spheres vs MaskImg is not implemented yet!");
			break;
		default:
			REPORT("TODO: throw new RuntimeException(cannot calculate distance!)");
			return 999.9f;
		}

		return 10.f;
	}


	void setAABB(AxisAlignedBoundingBox& AABB) const
	{
		//check centre plus/minus radius in every axis and record extremal coordinates
		//TODO
		REPORT("not implemented yet!");
	}


	// ------------- get/set methods -------------
	int getNoOfSpheres(void) const
	{
		return noOfSpheres;
	}

	const Vector3d<FLOAT>* getCentres(void) const
	{
		return centres;
	}

	const FLOAT* getRadii(void) const
	{
		return radii;
	}


	void updateCentre(const int i, const Vector3d<FLOAT>& centre)
	{
		centres[i] = centre;
	}

	void updateRadius(const int i, const FLOAT radius)
	{
		radii[i] = radius;
	}


	friend class NucleusAgent;
};
#endif
