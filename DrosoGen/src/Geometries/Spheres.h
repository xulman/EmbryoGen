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
	                 std::list<ProximityPair>& l) const override
	{
		switch (otherGeometry.shapeForm)
		{
		case ListOfShapeForms::Spheres:
			getDistanceToSpheres((Spheres*)&otherGeometry,l);
			break;
		case ListOfShapeForms::Mesh:
		case ListOfShapeForms::ScalarImg:
		case ListOfShapeForms::VectorImg:
			//find collision "from the other side"
			getSymmetricDistance(otherGeometry,l);
			break;

		case ListOfShapeForms::undefGeometry:
			REPORT("Ignoring other geometry of type 'undefGeometry'.");
			break;
		default:
			throw ERROR_REPORT("Not supported combination of shape representations.");
		}
	}

	/** Specialized implementation of getDistance() for Spheres-Spheres geometries.
	    For every non-zero-radius 'local' sphere, there is an 'other' sphere found
	    that has the nearest surface distance and corresponding ProximityPair is
	    added to the output list l.

	    Note that this may produce multiple 'local' spheres sharing the same
	    'other' sphere in their ProximityPairs. This can happen, for example,
	    when "T" configuration occurs or when large spheres-represented agent
	    is nearby. The 'local' agent should take this into account. */
	void getDistanceToSpheres(const Spheres* otherSpheres,
	                          std::list<ProximityPair>& l) const
	{
		//shortcuts to the otherGeometry's spheres
		const Vector3d<FLOAT>* const centresO = otherSpheres->getCentres();
		const FLOAT* const radiiO             = otherSpheres->getRadii();

		//for every my sphere: find nearest other sphere
		for (int im = 0; im < noOfSpheres; ++im)
		{
			//skip calculation for this sphere if it has no radius...
			if (radii[im] == 0) continue;

			//nearest other sphere discovered so far
			int bestIo = -1;
			FLOAT bestDist = TOOFAR;

			for (int io = 0; io < otherSpheres->getNoOfSpheres(); ++io)
			{
				//skip calculation for this sphere if it has no radius...
				if (radiiO[io] == 0) continue;

				//dist between surfaces of the two spheres
				FLOAT dist = (centres[im] - centresO[io]).len();
				dist -= radii[im] + radiiO[io];

				//is nearer?
				if (dist < bestDist)
				{
					bestDist = dist;
					bestIo   = io;
				}
			}

			//just in case otherSpheres->getNoOfSpheres() is 0...
			if (bestIo > -1)
			{
				//we have now a shortest distance between 'im' and 'bestIo',
				//create output ProximityPairs (and note indices of the incident spheres)

				//vector between the two centres (will be made
				//'radius' longer, and offsets the 'centre' point)
				Vector3d<FLOAT> dp = centresO[bestIo] - centres[im];
				dp.changeToUnitOrZero();

				l.emplace_back( centres[im] + (radii[im] * dp),
				                centresO[bestIo] - (radiiO[bestIo] * dp),
				                bestDist, im,bestIo );
			}
		}
	}

	/** tests position of the point w.r.t. this geometry, which is
	    the union of these spheres, and returns index of the first
	    sphere with which the point is in collision, or returns -1 if
	    there is no collision at all (with no sphere); the sphere at
	    the 'ignore' index is omitted from the tests (note the default
	    ignoreIdx is -1, which means to consider all spheres for the test) */
	int collideWithPoint(const Vector3d<FLOAT>& point,
	                     const int ignoreIdx = -1)
	{
		bool collision = false;

		Vector3d<FLOAT> tmp;
		int testingIndex = ignoreIdx == 0? 1 : 0;
		while (!collision && testingIndex < noOfSpheres)
		{
			tmp  = centres[testingIndex];
			tmp -= point;
			collision = tmp.len() <= radii[testingIndex];

			++testingIndex;
			if (testingIndex == ignoreIdx) ++testingIndex;
		}

		return collision == false ? -1 : testingIndex-1;
	}


	// ------------- AABB -------------
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override
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
	friend class Nucleus4SAgent;
	friend class SpheresFunctions;
};
#endif
