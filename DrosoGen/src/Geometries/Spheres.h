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

	/** Specialized implementation of getDistance() for Spheres-Spheres geometries.
	    For every non-zero-radius 'local' sphere, there is an 'other' sphere found
	    that has the nearest surface distance and corresponding ProximityPair is
	    added to the output list l. */
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
			int bestI = -1;
			FLOAT bestDist = TOOFAR;

			for (int io = 0; io < otherSpheres->getNoOfSpheres(); ++io)
			{
				//skip calculation for this sphere if it has no radius...
				if (radiiO[io] == 0) continue;

				//dist to centres //TODO: not sure: dc^2 - r1^2 - r2^2 =?= (dc -r1 -r2)^2
				//FLOAT dist = (centres[im] - centresO[io]).len2();
				//dist -= radii[im]*radii[im] + radiiO[io]*radiiO[io];

				//dist between surfaces of the two spheres
				FLOAT dist = (centres[im] - centresO[io]).len();
				dist -= radii[im] + radiiO[io];

				//is nearer?
				if (dist < bestDist)
				{
					bestDist = dist;
					bestI    = io;
				}
			}

			//just in case otherSpheres->getNoOfSpheres() is 0...
			if (bestI > -1)
			{
				//we have now a shortest distance between 'im' and 'bestI',
				//create output ProximityPairs
				ProximityPair p(centres[im],centresO[bestI], bestDist,
									 (void*)(centres+im),(void*)(centresO+bestI));

				//vector between the two centres
				Vector3d<FLOAT> dp = centresO[bestI] - centres[im];

				//the vector made 'radius' longer, and offets the 'centre' point
				p.localPos += (    +radii[im]/dp.len()) * dp;
				p.otherPos += (-radiiO[bestI]/dp.len()) * dp;

				//NB: a copy is of the ProximityPair 'p' is created while pushing...
				l.push_back(p);
			}
		}
	}


	// ------------- AABB -------------
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
