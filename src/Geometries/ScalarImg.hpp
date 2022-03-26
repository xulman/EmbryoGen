#pragma once

#include "Geometry.hpp"
#include <i3d/image3d.h>
class Spheres;

/**
 * Shape form based on image (given its size [px], offset [micrometers] and
 * resolution [px/micrometers]) where non-zero valued voxels represent each
 * a piece of a volume that is taken up by the represented agent. The set of
 * non-zero voxels need not form single connected component.
 *
 * When interacting with other objects, there are three modes available on
 * how to calculate distances:
 *
 * mask: 0001111000  - 1 represents inside the agent, 0 is outside of the agent
 *
 * a)    TT\____/TT  - distance inside is 0 everywhere, increases away from
 * object
 *
 * b)    TT\    /TT  - distance decreases & increases away from boundary of the
 * object
 *          \__/
 *
 * c)    ___    ___  - distance outside is 0 everywhere, decreases towards the
 * centre of the agent
 *          \__/
 *
 * Actually, the distance inside the object is reported with negative sign.
 * This way, an agent can distinguish how far it is from the "edge" (taking
 * absolute value of the distance) and on which "side of the edge" it is.
 *
 * Variant a) defines a shape/agent that does not distinguish within its volume,
 * making your agents "be afraid of non-zero distances" will make them stay
 * inside this geometry. Variant b) will make the agents stay along the
 * boundary/surface of this geometry. Variant c) will prevent agents from
 * staying inside this geometry.
 *
 * The class was originally designed with the assumption that agents who
 * interact with this agent will want to minimize their mutual (surface)
 * distance.
 *
 * However, the use of this class need not to be restricted to this
 * use-case -- in which case the best is to create a new class that
 * inherits from this one, overrides the getDistance() method, and
 * declares shapeForm = Geometry::ListOfShapeForms::undefGeometry.
 *
 * Author: Vladimir Ulman, 2018
 */
class ScalarImg : public Geometry {
  public:
	/** Defines how distances will be calculated on this geometry, see class
	 * ScalarImg docs */
	typedef enum {
		ZeroIN_GradOUT = 0,
		GradIN_GradOUT = 1,
		GradIN_ZeroOUT = 2
	} DistanceModel;

  private:
	/** Image with precomputed distances, it is of the same offset, size,
	   resolution (see docs of class ScalarImg) as the one given during
	   construction of this object */
	i3d::Image3d<float> distImg;

	/** (cached) resolution of the distImg [pixels per micrometer] */
	Vector3d<G_FLOAT> distImgRes;
	/** (cached) offset of the distImg's "minCorner" [micrometer] */
	Vector3d<G_FLOAT> distImgOff;
	/** (cached) offset of the distImg's "maxCorner" [micrometer] */
	Vector3d<G_FLOAT> distImgFarEnd;

	/** This is just a reminder of how the ScalarImg::distImg was created, since
	   we don't have reference or copy to the original source image and we
	   cannot reconstruct it easily... */
	const DistanceModel model;

  public:
	/** constructor can be a bit more memory expensive if _model is
	 * GradIN_GradOUT */
	template <class MT>
	ScalarImg(const i3d::Image3d<MT>& _mask, DistanceModel _model)
	    : Geometry(ListOfShapeForms::ScalarImg), model(_model) {
		updateWithNewMask(_mask);
	}

  private:
	/** private c'tor only for createAndDeserializeFrom() */
	ScalarImg(DistanceModel _model)
	    : Geometry(ListOfShapeForms::ScalarImg),
	      model(_model) { /** createAndDeserializeFrom() will do it for us */
	}

  public:
	/** just for debug purposes: save the distance image to a filename */
	void saveDistImg(const char* filename) { distImg.SaveImage(filename); }

	// ------------- distances -------------
	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const override;

	/** Specialized implementation of getDistance() for ScalarImg & Spheres
	   geometries. Rasterizes the 'other' spheres into the 'local' ScalarImg and
	   finds min distance for every other sphere. These nearest surface
	   distances and corresponding ProximityPairs are added to the output list
	   l.

	    If a Sphere is calculating distance to a ScalarImg, the ProximityPair
	   "points" (the vector from ProximityPair::localPos to
	   ProximityPair::otherPos) from Sphere's surface in the direction parallel
	   to the gradient (determined at 'localPos' in the ScalarImg::distImg)
	   towards a surface of the ScalarImg. The surface, at 'otherPos', is not
	   necessarily accurate as it is estimated from the distance and (local!)
	    gradient only. The length of such a "vector" is the same as a distance
	   to the surface found at 'localPos' in the 'distImg'. If a ScalarImg is
	   calculating distance to a Sphere, the opposite "vector" is created and
	   placed such that the tip of this "vector" points at Sphere's surface. In
	   other words, the tip becomes the base and vice versa. */
	void getDistanceToSpheres(const class Spheres* otherSpheres,
	                          std::list<ProximityPair>& l) const;

	/** Specialized implementation of getDistance() for ScalarImg-ScalarImg
	 * geometries. */
	/*
	void getDistanceToScalarImg(const ScalarImg* otherScalarImg,
	                            std::list<ProximityPair>& l) const;
	*/

	/** Specialized implementation of getDistance() for ScalarImg-VectorImg
	 * geometries. */
	/*
	void getDistanceToVectorImg(const VectorImg* otherVectorImg,
	                            std::list<ProximityPair>& l) const;
	*/

	// ------------- AABB -------------
	/** construct AABB from the the mask image considering
	    only non-zero valued voxels, and considering mask's
	    offset and resolution */
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override;

	// ------------- get/set methods -------------
	const i3d::Image3d<float>& getDistImg(void) const { return distImg; }

	const Vector3d<G_FLOAT>& getDistImgRes(void) const { return distImgRes; }

	const Vector3d<G_FLOAT>& getDistImgOff(void) const { return distImgOff; }

	const Vector3d<G_FLOAT>& getDistImgFarEnd(void) const {
		return distImgFarEnd;
	}

	DistanceModel getDistImgModel(void) const { return model; }

	template <class MT>
	void updateWithNewMask(const i3d::Image3d<MT>& _mask);

  private:
	/** updates ScalarImg::distImgOff, ScalarImg::distImgRes and
	   ScalarImg::distImgFarEnd to the current ScalarImg::distImg */
	void updateDistImgResOffFarEnd(void) {
		distImgRes.fromI3dVector3d(distImg.GetResolution().GetRes());

		//"min" corner
		distImgOff.fromI3dVector3d(distImg.GetOffset());

		// this mask image's "max" corner in micrometers
		distImgFarEnd.from(Vector3d<size_t>(distImg.GetSize()))
		    .toMicrons(distImgRes, distImgOff);
	}

	// ----------------- support for serialization and deserealization
	// -----------------
  public:
	long getSizeInBytes() const override;

	void serializeTo(char* buffer) const override;
	void deserializeFrom(char* buffer) override;

	static ScalarImg* createAndDeserializeFrom(char* buffer);

	// ----------------- support for rasterization -----------------
	void renderIntoMask(i3d::Image3d<i3d::GRAY16>& mask,
	                    const i3d::GRAY16 drawID) const override;
};
