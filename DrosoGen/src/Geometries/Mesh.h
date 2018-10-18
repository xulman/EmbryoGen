#ifndef MESH_H
#define MESH_H

#include "../util/report.h"
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


	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const override
	{
		switch (otherGeometry.shapeForm)
		{
		case ListOfShapeForms::Spheres:
			//TODO: attempt to project mesh vertices against the spheres (or their AABB) and look for collision
			REPORT("this.Mesh vs Spheres is not implemented yet!");
			break;
		case ListOfShapeForms::Mesh:
			//TODO identity case
			REPORT("this.Mesh vs Mesh is not implemented yet!");
			break;
		case ListOfShapeForms::ScalarImg:
			//find collision "from the other side"
			getSymmetricDistance(otherGeometry,l);
			break;
		default:
			throw new std::runtime_error("Geometry::getDistance(): Not supported combination of shape representations.");
		}
	}


	/** construct AABB from the given Mesh */
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override
	{
		//scan through the mesh vertices/nodes and find extremal coordinates
		//TODO
		REPORT("not implemented yet!");
	}
};
#endif
