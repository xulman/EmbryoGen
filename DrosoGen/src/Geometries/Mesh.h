#ifndef MESH_H
#define MESH_H

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


	/** calculate min distance between myself and some foreign agent */
	std::list<ProximityPair>*
	getDistance(const Geometry& otherGeometry) const override
	{
		//default return value
		std::list<ProximityPair>* l = emptyCollisionListPtr;

		switch (otherGeometry.shapeForm)
		{
		case ListOfShapeForms::Spheres:
			//TODO: attempt to project mesh vertices against the spheres and look for collision
			REPORT("this.Mesh vs Spheres is not implemented yet!");
			break;
		case ListOfShapeForms::Mesh:
			//TODO identity case
			REPORT("this.Mesh vs Mesh is not implemented yet!");
			break;
		case ListOfShapeForms::MaskImg:
			//TODO: attempt to project mesh vertices into the mask image and look for collision
			REPORT("this.Mesh vs MaskImg is not implemented yet!");
			break;
		default:
			throw new std::runtime_error("Geometry::getDistance(): Not supported combination of shape representations.");
		}

		return l;
	}


	/** construct AABB from the given Mesh */
	void setAABB(AxisAlignedBoundingBox& AABB) const override
	{
		//scan through the mesh vertices/nodes and find extremal coordinates
		//TODO
		REPORT("not implemented yet!");
	}
};
#endif
