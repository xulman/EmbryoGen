#ifndef MESH_H
#define MESH_H

#include "Geometry.h"

class Mesh: public Geometry
{
private:
	//TODO: attribute of type mesh

public:
	Mesh(void): shapeStyle(Mesh)
	{
		//TODO, somehow create this.mesh
	}

	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Spheres& spheres) const
	{
		//TODO: attempt to project mesh vertices against the spheres and look for collision
		REPORT("this.Mesh vs Spheres is not implemented yet!");
	}

	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Mesh& mesh) const
	{
		//TODO
		REPORT("this.Mesh vs Mesh is not implemented yet!");
	}

	/** calculate min distance between myself and some foreign agent */
	template <class FMT> //Foreign MT
	FLOAT getDistance(const MaskImg<FMT>& mask) const
	{
		//TODO: attempt to project mesh vertices into the mask image and look for collision
		REPORT("this.Mesh vs MaskImg is not implemented yet!");
	}

	///construct AABB from the given Mesh
	void setAABB(AxisAlignedBoundingBox& AABB) const
	{
		//scan through the mesh vertices/nodes and find extremal coordinates
		//TODO
		REPORT("not implemented yet!");
	}
}
#endif
