#ifndef GEOMETRY_MESH_H
#define GEOMETRY_MESH_H

#include "Geometry.h"

/**
 * TBA
 *
 * Author: Vladimir Ulman, 2018
 */
class Mesh: public Geometry
{
private:
	//TODO: attribute of type mesh

public:
	Mesh(void): Geometry(ListOfShapeForms::Mesh)
	{
		//TODO, somehow create this.mesh
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
};
#endif