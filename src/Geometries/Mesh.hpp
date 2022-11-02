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
	void getDistance(
	    const Geometry& otherGeometry,
	    tools::structures::SmallVector5<ProximityPair>& l) const override;

	// ------------- AABB -------------
	/** construct AABB from the given Mesh */
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override;

	// ------------- get/set methods -------------

	// ----------------- support for serialization and deserealization
	// -----------------
  public:
	long getSizeInBytes() const override;

	std::vector<std::byte> serialize() const override;
	void deserialize(std::span<const std::byte> buffer) override;

	static Mesh createAndDeserialize(std::span<const std::byte> buffer);

	// ----------------- support for rasterization -----------------
	void renderIntoMask(i3d::Image3d<i3d::GRAY16>& mask,
	                    const i3d::GRAY16 drawID) const override;
};
