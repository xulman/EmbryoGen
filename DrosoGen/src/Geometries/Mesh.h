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
	Mesh(void): Geometry(ListOfShapes::Mesh)
	{
		//TODO, somehow create this.mesh
	}


	/** calculate min distance between myself and some foreign agent */
	std::list<ProximityPair>*
	getDistance(const Geometry& otherGeometry) const
	{
		//default return value
		std::list<ProximityPair>* l = emptyCollisionListPtr;

		switch (otherGeometry.shapeStyle)
		{
		case ListOfShapes::Spheres:
			//TODO: attempt to project mesh vertices against the spheres and look for collision
			REPORT("this.Mesh vs Spheres is not implemented yet!");
			break;
		case ListOfShapes::Mesh:
			//TODO identity case
			REPORT("this.Mesh vs Mesh is not implemented yet!");
			break;
		case ListOfShapes::MaskImg:
			//TODO: attempt to project mesh vertices into the mask image and look for collision
			REPORT("this.Mesh vs MaskImg is not implemented yet!");
			break;
		default:
			REPORT("TODO: throw new RuntimeException(cannot calculate distance!)");
		}

		return l;
	}


	///construct AABB from the given Mesh
	void setAABB(AxisAlignedBoundingBox& AABB) const
	{
		//scan through the mesh vertices/nodes and find extremal coordinates
		//TODO
		REPORT("not implemented yet!");
	}
};
#endif
