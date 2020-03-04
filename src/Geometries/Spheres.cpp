#include "Spheres.h"

void Spheres::getDistance(const Geometry& otherGeometry,
                          std::list<ProximityPair>& l) const
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


void Spheres::getDistanceToSpheres(const Spheres* otherSpheres,
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


int Spheres::collideWithPoint(const Vector3d<FLOAT>& point,
                              const int ignoreIdx)
const
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


void Spheres::updateThisAABB(AxisAlignedBoundingBox& AABB) const
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


long Spheres::getSizeInBytes() const
{
	return sizeof(int) + noOfSpheres * 4 * sizeof(FLOAT);
}
