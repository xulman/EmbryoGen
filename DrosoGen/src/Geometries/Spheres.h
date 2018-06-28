#ifndef SPHERES_H
#define SPHERES_H

#include "../util/report.h"
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
	/** length of the this.centres and this.radii arrays */
	const int noOfSpheres;

	/** list of centres of the spheres */
	Vector3d<FLOAT>* const centres;

	/** list of radii of the spheres */
	FLOAT* const radii;

public:
	/** empty shape constructor */
	Spheres(const int _noOfSpheres)
		: Geometry(ListOfShapeForms::Spheres),
		  noOfSpheres(_noOfSpheres),
		  centres(new Vector3d<FLOAT>[noOfSpheres]),
		  radii(new FLOAT[noOfSpheres])
	{
		for (int i=0; i < noOfSpheres; ++i) radii[i] = 0.0;
	}

	/** copy constructor */
	Spheres(const Spheres& s)
		: Geometry(ListOfShapeForms::Spheres),
		  noOfSpheres(s.getNoOfSpheres()),
		  centres(new Vector3d<FLOAT>[noOfSpheres]),
		  radii(new FLOAT[noOfSpheres])
	{
		const Vector3d<FLOAT>* sCentres = s.getCentres();
		const FLOAT*           sRadii   = s.getRadii();

		for (int i=0; i < noOfSpheres; ++i)
		{
			centres[i] = sCentres[i];
			radii[i]   = sRadii[i];
		}
	}


	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const override
	{
		switch (otherGeometry.shapeForm)
		{
		case ListOfShapeForms::Spheres:
			//TODO identity case
			REPORT("this.Spheres vs Spheres is not implemented yet!");
			break;
		case ListOfShapeForms::Mesh:
			//find collision "from the other side"
			getSymmetricDistance(otherGeometry,l);
			break;
		case ListOfShapeForms::MaskImg:
			//find collision "from the other side"
			getSymmetricDistance(otherGeometry,l);
			break;
		default:
			throw new std::runtime_error("Geometry::getDistance(): Not supported combination of shape representations.");
		}
	}


	void setAABB(AxisAlignedBoundingBox& AABB) const override
	{
		AABB.reset();

		//check centre plus/minus radius in every axis and record extremal coordinates
		for (int i=0; i < noOfSpheres; ++i)
		if (radii[i] > 0.f)
		{
			AABB.minCorner.x = std::min(AABB.minCorner.x, centres[i].x-radii[i]);
			AABB.maxCorner.x = std::max(AABB.maxCorner.x, centres[i].x+radii[i]);

			AABB.minCorner.y = std::min(AABB.minCorner.y, centres[i].y-radii[i]);
			AABB.maxCorner.y = std::max(AABB.maxCorner.y, centres[i].y+radii[i]);

			AABB.minCorner.z = std::min(AABB.minCorner.z, centres[i].z-radii[i]);
			AABB.maxCorner.z = std::max(AABB.maxCorner.z, centres[i].z+radii[i]);
		}
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
