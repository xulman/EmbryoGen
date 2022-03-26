#pragma once

#include "Geometry.hpp"

/**
 * TBA
 *
 * Author: Vladimir Ulman, 2018
 */
class Mesh : public Geometry {
  private:
	// TODO: attribute of type mesh

  public:
	Mesh(void) : Geometry(ListOfShapeForms::Mesh) {
		// TODO, somehow create this.mesh
	}

	/** copy constructor */
	/*
	Mesh(const Mesh& s): Geometry(ListOfShapeForms::Mesh)
	{
	    //TODO, somehow copy if there are new() used in the main c'tor
	}
	*/

	// ------------- distances -------------
	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const override;

	// ------------- AABB -------------
	/** construct AABB from the given Mesh */
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override;

	// ------------- get/set methods -------------

	// ----------------- support for serialization and deserealization
	// -----------------
  public:
	long getSizeInBytes() const override;

	void serializeTo(char* buffer) const override;
	void deserializeFrom(char* buffer) override;

	static Mesh* createAndDeserializeFrom(char* buffer);

	// ----------------- support for rasterization -----------------
	void renderIntoMask(i3d::Image3d<i3d::GRAY16>& mask,
	                    const i3d::GRAY16 drawID) const override;
};
