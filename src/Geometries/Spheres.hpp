#pragma once

#include "../../extern/mpi_wrapper/src/include.hpp"
#include "../util/report.hpp"
#include "Geometry.hpp"
#include <cassert>
#include <vector>
/**
 * Collection of Spheres::noOfSpheres spheres, the represented shape/geometry
 * is given as the union of these spheres.
 *
 * Author: Vladimir Ulman, 2018
 */
class Spheres : public Geometry {
  protected:
	/** centers and radii sizes are always the same */

	/** list of centres of the spheres */
	std::vector<Vector3d<precision_t>> centres;

	/** list of radii of the spheres */
	std::vector<precision_t> radii;

  public:
	/** empty shape constructor */
	Spheres(const int noOfSpheres)
	    : Geometry(ListOfShapeForms::Spheres), centres(noOfSpheres),
	      radii(noOfSpheres) {
		// sanity check...
		if (noOfSpheres < 0)
			throw report::rtError(
			    "Cannot construct geometry with negative number of spheres.");

		// report::message(fmt::format("Constructing spheres @ {}" , this));
		// report::message(fmt::format("Obtaining new arrays in spheres @ {}" ,
		// this));
	}

	Spheres(std::vector<Vector3d<precision_t>> centres,
	        std::vector<precision_t> radii)
	    : Geometry(ListOfShapeForms::Spheres), centres(std::move(centres)),
	      radii(std::move(radii)) {
		// sanity check...
		assert(centres.size() == radii.size());
	}

	// ------------- distances -------------
	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(
	    const Geometry& otherGeometry,
	    tools::structures::SmallVector5<ProximityPair>& l) const override;

	/** Specialized implementation of getDistance() for Spheres-Spheres
	   geometries. For every non-zero-radius 'local' sphere, there is an 'other'
	   sphere found that has the nearest surface distance and corresponding
	   ProximityPair is added to the output list l.

	    Note that this may produce multiple 'local' spheres sharing the same
	    'other' sphere in their ProximityPairs. This can happen, for example,
	    when "T" configuration occurs or when large spheres-represented agent
	    is nearby. The 'local' agent should take this into account. */
	void getDistanceToSpheres(
	    const Spheres* otherSpheres,
	    tools::structures::SmallVector5<ProximityPair>& l) const;

	/** tests position of the point w.r.t. this geometry, which is
	    the union of these spheres, and returns index of the first
	    sphere with which the point is in collision, or returns -1 if
	    there is no collision at all (with no sphere); the sphere at
	    the 'ignore' index is omitted from the tests (note the default
	    ignoreIdx is -1, which means to consider all spheres for the test) */
	int collideWithPoint(const Vector3d<precision_t>& point,
	                     const int ignoreIdx = -1) const;

	// ------------- AABB -------------
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override;

	// ------------- get/set methods -------------
	std::size_t getNoOfSpheres() const {
		assert(centres.size() == radii.size());
		return centres.size();
	}

	const std::vector<Vector3d<precision_t>>& getCentres(void) const {
		return centres;
	}

	const std::vector<precision_t>& getRadii(void) const { return radii; }

	void updateCentre(const int i, const Vector3d<precision_t>& centre) {
		centres[i] = centre;
	}

	void updateRadius(const int i, const precision_t radius) {
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

	// ----------------- support for serialization and deserealization
	// -----------------
  public:
	long getSizeInBytes() const override;

	std::vector<std::byte> serialize() const override;
	void deserialize(std::span<const std::byte> buffer) override;

	static Spheres createAndDeserialize(std::span<const std::byte> buffer);

	// ----------------- support for rasterization -----------------
	void renderIntoMask(i3d::Image3d<i3d::GRAY16>& mask,
	                    const i3d::GRAY16 drawID) const override;
};
