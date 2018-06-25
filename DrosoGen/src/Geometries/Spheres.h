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
	Vector3d<FLOAT> centres[noOfSpheres];

	///list of radii of the spheres
	FLOAT radii[noOfSpheres];

public:
	Spheres(const int _noOfSpheres): shapeStyle(Spheres)
	{
		noOfSpheres = _noOfSpheres;
		for (int i=0; i < noOfSpheres; ++i) radii[i] = 0.0;
	}

	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Spheres& spheres) const
	{
		//TODO
		REPORT("this.Spheres vs Spheres is not implemented yet!");
	}

	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Mesh& mesh) const
	{
		return mesh.getDistance(*this);
	}

	/** calculate min distance between myself and some foreign agent */
	template <class FMT> //Foreign MT
	FLOAT getDistance(const MaskImg<FMT>& mask) const
	{
		//TODO: attempt to render spheres into the mask image and look for collision
		REPORT("this.Spheres vs MaskImg is not implemented yet!");
	}

	void setAABB(AxisAlignedBoundingBox& AABB) const
	{
		//check centre plus/minus radius in every axis and record extremal coordinates
		//TODO
		REPORT("not implemented yet!");
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
}
#endif
