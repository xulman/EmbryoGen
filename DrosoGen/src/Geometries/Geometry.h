#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "../vector3d.h"

/// accuracy of the geometry representation
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

	/*
	///construct AABB from the given geometry
	AxisAlignedBoundingBox(const Geometry& shape)
	{
		update(shape);
	}
	*/

	///resets the BB (before one starts filling it)
	void inline reset(void)
	{
		minCorner = FLOAT(+999999999.0);
		maxCorner = FLOAT(-999999999.0);
	}

	/*
	void inline update(const Geometry& shape)
	{
		shape.setAABB(*this);
	}
	*/
};


/** A common ancestor class/type for the Spheres, Mesh and Mask Image
    representations of agent's geometry. It defines (empty) methods to
    calculate distance between agents. */
class Geometry
{
protected:
	/** A variant of how the shape of an simulated agent is represented */
	typedef enum
	{
		Spheres=0,
		Mesh=1,
		MaskImg=2
	} ListOfShapes;

	Geometry(const ListOfShapes _shapeStyle) : shapeStyle(_shapeStyle), AABB() {};


public:
	const ListOfShapes shapeStyle;
	AxisAlignedBoundingBox AABB;

	/** calculate min distance between myself and some foreign agent */
	virtual
	FLOAT getDistance(const Geometry& otherGeometry) const =0;

	/** sets the given AABB to reflect the current geometry */
	virtual
	void setAABB(AxisAlignedBoundingBox& AABB) const =0;

	/** sets this object's own AABB to reflect the current geometry */
	void setAABB(void)
	{
		setAABB(this->AABB);
	}
};
#endif
