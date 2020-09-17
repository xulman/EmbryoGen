#ifndef GEOMETRY_SPHERES_H
#define GEOMETRY_SPHERES_H

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
protected:
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
		//sanity check...
		if (_noOfSpheres < 0)
			throw ERROR_REPORT("Cannot construct geometry with negative number of spheres.");

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

	~Spheres(void)
	{
		//c'tor guarantees that these arrays were always created
		delete[] centres;
		delete[] radii;
	}


	// ------------- distances -------------
	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const override;

	/** Specialized implementation of getDistance() for Spheres-Spheres geometries.
	    For every non-zero-radius 'local' sphere, there is an 'other' sphere found
	    that has the nearest surface distance and corresponding ProximityPair is
	    added to the output list l.

	    Note that this may produce multiple 'local' spheres sharing the same
	    'other' sphere in their ProximityPairs. This can happen, for example,
	    when "T" configuration occurs or when large spheres-represented agent
	    is nearby. The 'local' agent should take this into account. */
	void getDistanceToSpheres(const Spheres* otherSpheres,
	                          std::list<ProximityPair>& l) const;


	/** tests position of the point w.r.t. this geometry, which is
	    the union of these spheres, and returns index of the first
	    sphere with which the point is in collision, or returns -1 if
	    there is no collision at all (with no sphere); the sphere at
	    the 'ignore' index is omitted from the tests (note the default
	    ignoreIdx is -1, which means to consider all spheres for the test) */
	int collideWithPoint(const Vector3d<FLOAT>& point,
	                     const int ignoreIdx = -1) const;


	// ------------- AABB -------------
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override;


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


	friend class SpheresFunctions;
	friend class NucleusAgent;
	friend class Nucleus4SAgent;
	friend class Nucleus2pNSAgent;
	friend class Texture;
	friend class TextureQuantized;
	friend class TextureUpdater4S;
	friend class TextureUpdater2pNS;

	// ----------------- support for serialization and deserealization -----------------
public:
	long getSizeInBytes() const override;

	void serializeTo(char* buffer) const override;
	void deserializeFrom(char* buffer) override;

	static Spheres* createAndDeserializeFrom(char* buffer);
};
#endif
