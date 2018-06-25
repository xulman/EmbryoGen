#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "../vector3d.h"
#include <image3d.h>

#define FLOAT float

/** An x,y,z-axes aligned 3D bounding box for approximate representation of agent's geometry */
class AxisAlignedBoundingBox
{
public:
	///defines the bounding box with its volumetric diagonal
	Vector3d<FLOAT> minCorner;
	Vector3d<FLOAT> maxCorner;

	///construct an empty AABB
	AxisAlignedBoundingBox(void)
	{
		reset();
	}

	///construct AABB from the given geometry
	AxisAlignedBoundingBox(const Geometry& shape)
	{
		update(shape);
	}

	///resets the BB (before one starts filling it)
	void inline reset(void)
	{
		minCorner = +999999999.0;
		maxCorner = -999999999.0;
	}

	void inline update(const Geometry& shape)
	{
		shape.setAABB(*this);
	}
};


/** A variant of how the shape of an simulated agent is represented */
typedef enum
{
	Spheres=0,
	Mesh=1,
	MaskImg=2
} ListOfShapes;


/** A common ancestor class/type for the Spheres, Mesh and Mask Image
    representations of agent's geometry. It defines (empty) methods to
    calculate distance between agents. */
class Geometry
{
public:
	const ListOfShapes shapeStyle;
	AxisAlignedBoundingBox AABB;

	/** calculate min distance between myself and some foreign agent */
	virtual
	FLOAT getDistance(const Spheres& spheres) const =0;

	/** calculate min distance between myself and some foreign agent */
	virtual
	FLOAT getDistance(const Mesh& mesh) const =0;

	/** calculate min distance between myself and some foreign agent */
	virtual
	template <class FMT> //Foreign MT
	FLOAT getDistance(const MaskImg<FMT>& mask) const =0;

	virtual
	void setAABB(AxisAlignedBoundingBox& AABB) const =0;

	void setAABB(void)
	{
		setAABB(this->AABB);
	}
};
#endif
