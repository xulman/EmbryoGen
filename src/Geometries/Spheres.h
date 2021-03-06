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
	Vector3d<G_FLOAT>* const centres;

	/** list of radii of the spheres */
	G_FLOAT* const radii;

	/** a flag to prevent this object from freeing both 'centres' and 'radii'
	    attributes in its (*this) destructor; this flag is set only in its
	    (*this) moving constructor to signal arrays are "stolen" (technically,
	    shared) into another object; note both attribs are immutable... */
	bool dataMovedAwayDontDelete = false;

public:
	/** empty shape constructor */
	Spheres(const int _noOfSpheres)
		: Geometry(ListOfShapeForms::Spheres),
		  noOfSpheres(_noOfSpheres),
		  centres(new Vector3d<G_FLOAT>[noOfSpheres]),
		  radii(new G_FLOAT[noOfSpheres])
	{
		//sanity check...
		if (_noOfSpheres < 0)
			throw ERROR_REPORT("Cannot construct geometry with negative number of spheres.");

		for (int i=0; i < noOfSpheres; ++i) radii[i] = 0.0;
		//REPORT("Constructing spheres @ " << this);
		//REPORT("Obtaining new arrays in spheres @ " << this);
	}

	/** move constructor */
	Spheres(Spheres&& s)
		: Geometry(ListOfShapeForms::Spheres),
		  noOfSpheres(s.noOfSpheres),
		  centres(s.centres),
		  radii(s.radii)
	{
		s.dataMovedAwayDontDelete = true;
		//REPORT( "/ Moving spheres from " << &s);
		//REPORT("\\ Moving spheres into " << this);
		//REPORT("Stealing arrays into spheres @ " << this);
	}

	/** copy constructor */
	Spheres(const Spheres& s)
		: Geometry(ListOfShapeForms::Spheres),
		  noOfSpheres(s.getNoOfSpheres()),
		  centres(new Vector3d<G_FLOAT>[noOfSpheres]),
		  radii(new G_FLOAT[noOfSpheres])
	{
		const Vector3d<G_FLOAT>* sCentres = s.getCentres();
		const G_FLOAT*           sRadii   = s.getRadii();

		for (int i=0; i < noOfSpheres; ++i)
		{
			centres[i] = sCentres[i];
			radii[i]   = sRadii[i];
		}
		//REPORT( "/ Copying spheres from " << &s);
		//REPORT("\\ Copying spheres into " << this);
		//REPORT("Duplicating arrays into spheres @ " << this);
	}

	~Spheres(void)
	{
		//free only if we still "own" both arrays
		if (dataMovedAwayDontDelete == false)
		{
			delete[] centres;
			delete[] radii;
			//REPORT("Freeing arrays in spheres @ " << this);
		}
		//REPORT("Destructing spheres @ " << this);
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
	int collideWithPoint(const Vector3d<G_FLOAT>& point,
	                     const int ignoreIdx = -1) const;


	// ------------- AABB -------------
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override;


	// ------------- get/set methods -------------
	int getNoOfSpheres(void) const
	{
		return noOfSpheres;
	}

	const Vector3d<G_FLOAT>* getCentres(void) const
	{
		return centres;
	}

	const G_FLOAT* getRadii(void) const
	{
		return radii;
	}


	void updateCentre(const int i, const Vector3d<G_FLOAT>& centre)
	{
		centres[i] = centre;
	}

	void updateRadius(const int i, const G_FLOAT radius)
	{
		radii[i] = radius;
	}


	friend class SpheresFunctions;
	friend class NucleusAgent;
	friend class Nucleus4SAgent;
	friend class NucleusNSAgent;
	friend class Texture;
	friend class TextureQuantized;
	friend class TextureUpdater4S;
	friend class TextureUpdaterNS;
	friend class TextureUpdater2pNS;

	// ----------------- support for serialization and deserealization -----------------
public:
	long getSizeInBytes() const override;

	void serializeTo(char* buffer) const override;
	void deserializeFrom(char* buffer) override;

	static Spheres* createAndDeserializeFrom(char* buffer);

	// ----------------- support for rasterization -----------------
	void renderIntoMask(i3d::Image3d<i3d::GRAY16>& mask, const i3d::GRAY16 drawID) const override;
};
#endif
