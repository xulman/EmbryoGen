#include "Spheres.hpp"
#include "util/Serialization.hpp"

void Spheres::getDistance(const Geometry& otherGeometry,
                          std::list<ProximityPair>& l) const {
	switch (otherGeometry.shapeForm) {
	case ListOfShapeForms::Spheres:
		getDistanceToSpheres((Spheres*)&otherGeometry, l);
		break;
	case ListOfShapeForms::Mesh:
	case ListOfShapeForms::ScalarImg:
	case ListOfShapeForms::VectorImg:
		// find collision "from the other side"
		getSymmetricDistance(otherGeometry, l);
		break;

	case ListOfShapeForms::undefGeometry:
		report::message(
		    fmt::format("Ignoring other geometry of type 'undefGeometry'."));
		break;
	default:
		throw report::rtError(
		    "Not supported combination of shape representations.");
	}
}

void Spheres::getDistanceToSpheres(const Spheres* otherSpheres,
                                   std::list<ProximityPair>& l) const {
	// shortcuts to the otherGeometry's spheres
	const Vector3d<G_FLOAT>* const centresO = otherSpheres->getCentres();
	const G_FLOAT* const radiiO = otherSpheres->getRadii();

	// for every my sphere: find nearest other sphere
	for (std::size_t im = 0; im < getNoOfSpheres(); ++im) {
		// skip calculation for this sphere if it has no radius...
		if (radii[im] == 0)
			continue;

		// nearest other sphere discovered so far
		int bestIo = -1;
		G_FLOAT bestDist = TOOFAR;

		for (std::size_t io = 0; io < otherSpheres->getNoOfSpheres(); ++io) {
			// skip calculation for this sphere if it has no radius...
			if (radiiO[io] == 0)
				continue;

			// dist between surfaces of the two spheres
			G_FLOAT dist = (centres[im] - centresO[io]).len();
			dist -= radii[im] + radiiO[io];

			// is nearer?
			if (dist < bestDist) {
				bestDist = dist;
				bestIo = int(io);
			}
		}

		// just in case otherSpheres->getNoOfSpheres() is 0...
		if (bestIo > -1) {
			// we have now a shortest distance between 'im' and 'bestIo',
			// create output ProximityPairs (and note indices of the incident
			// spheres)

			// vector between the two centres (will be made
			//'radius' longer, and offsets the 'centre' point)
			Vector3d<G_FLOAT> dp = centresO[bestIo] - centres[im];
			dp.changeToUnitOrZero();

			l.emplace_back(centres[im] + (radii[im] * dp),
			               centresO[bestIo] - (radiiO[bestIo] * dp), bestDist,
			               im, bestIo);
		}
	}
}

int Spheres::collideWithPoint(const Vector3d<G_FLOAT>& point,
                              const int ignoreIdx) const {
	bool collision = false;

	Vector3d<G_FLOAT> tmp;
	std::size_t testingIndex = ignoreIdx == 0 ? 1 : 0;
	while (!collision && testingIndex < radii.size()) {
		tmp = centres[testingIndex];
		tmp -= point;
		collision = tmp.len() <= radii[testingIndex];

		++testingIndex;
		if (int(testingIndex) == ignoreIdx)
			++testingIndex;
	}

	return !collision ? -1 : int(testingIndex) - 1;
}

void Spheres::updateThisAABB(AxisAlignedBoundingBox& AABB) const {
	AABB.reset();

	// check centre plus/minus radius in every axis and record extremal
	// coordinates
	for (std::size_t i = 0; i < getNoOfSpheres(); ++i)
		if (radii[i] > 0.f) {
			AABB.minCorner.x =
			    std::min(AABB.minCorner.x, centres[i].x - radii[i]);
			AABB.maxCorner.x =
			    std::max(AABB.maxCorner.x, centres[i].x + radii[i]);

			AABB.minCorner.y =
			    std::min(AABB.minCorner.y, centres[i].y - radii[i]);
			AABB.maxCorner.y =
			    std::max(AABB.maxCorner.y, centres[i].y + radii[i]);

			AABB.minCorner.z =
			    std::min(AABB.minCorner.z, centres[i].z - radii[i]);
			AABB.maxCorner.z =
			    std::max(AABB.maxCorner.z, centres[i].z + radii[i]);
		}
}

// ----------------- support for serialization and deserealization
// -----------------
long Spheres::getSizeInBytes() const {
	return sizeof(int) + 
		+ sizeof(decltype(version)) 
		+ centres.size() * sizeof(G_FLOAT) * 3 
		+ radii.size() * sizeof(G_FLOAT);
}

void Spheres::serializeTo(char* buffer) const {
	// store noOfSpheres
	long off = Serialization::toBuffer(int(getNoOfSpheres()), buffer);

	// store individual spheres
	for (std::size_t i = 0; i < getNoOfSpheres(); ++i) {
		off += Serialization::toBuffer(centres[i], buffer + off);
		off += Serialization::toBuffer(radii[i], buffer + off);
	}

	Serialization::toBuffer(version, buffer + off);
}

void Spheres::deserializeFrom(char* buffer) {
	int recv_noOfSpheres;
	long off = Deserialization::fromBuffer(buffer, recv_noOfSpheres);
	std::size_t noOfSpheres = getNoOfSpheres();

	if (int(noOfSpheres) != recv_noOfSpheres)
		throw report::rtError(
		    fmt::format("Deserialization mismatch: cannot fill geometry of {} "
		                "spheres from the buffer with {} spheres",
		                noOfSpheres, recv_noOfSpheres));

	// read and setup individual spheres
	for (std::size_t i = 0; i < noOfSpheres; ++i) {
		off += Deserialization::fromBuffer(buffer + off, centres[i]);
		off += Deserialization::fromBuffer(buffer + off, radii[i]);
	}

	// update Geometry attribs:
	Deserialization::fromBuffer(buffer + off, version);
	updateThisAABB(this->AABB);
}

Spheres* Spheres::createAndDeserializeFrom(char* buffer) {
	// read noOfSpheres and create an object
	Spheres* s = new Spheres(*((int*)buffer));
	s->deserializeFrom(buffer);
	return s;
}

void Spheres::renderIntoMask(i3d::Image3d<i3d::GRAY16>& mask,
                             const i3d::GRAY16 drawID) const {
	// shortcuts to the mask image parameters
	const Vector3d<G_FLOAT> res(mask.GetResolution().GetRes());
	const Vector3d<G_FLOAT> off(mask.GetOffset());

	// project and "clip" this AABB into the img frame
	// so that voxels to sweep can be narrowed down...
	//
	//    sweeping position and boundaries (relevant to the 'mask')
	Vector3d<size_t> curPos, minSweepPX, maxSweepPX;
	AABB.exportInPixelCoords(mask, minSweepPX, maxSweepPX);
	//
	// micron coordinate of the running voxel 'curPos'
	Vector3d<G_FLOAT> centre;

	// sweep and check intersection with spheres' volumes
	for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
		for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
			for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++) {
				// get micron coordinate of the current voxel's centre
				centre.toMicronsFrom(curPos, res, off);

				// check the current voxel against all spheres
				for (std::size_t i = 0; i < getNoOfSpheres(); ++i) {
					if ((centre - centres[i]).len() <= radii[i]) {
#ifndef NDEBUG
						i3d::GRAY16 val =
						    mask.GetVoxel(curPos.x, curPos.y, curPos.z);
						if (val > 0 && val != drawID)
							report::message(
							    fmt::format("{} overwrites mask of {} at {}",
							                drawID, val, toString(curPos)));
#endif
						mask.SetVoxel(curPos.x, curPos.y, curPos.z, drawID);
						break; // no need to test against the remaining spheres
					}
				}
			}
}
