#ifndef MASKIMG_H
#define MASKIMG_H

#include <i3d/image3d.h>
#include "Geometry.h"

/**
 * TBA
 *
 * Author: Vladimir Ulman, 2018
 */
template <class MT>
class MaskImg: public Geometry
{
private:
	i3d::Image3d<MT> mask;

public:
	MaskImg(void): Geometry(ListOfShapes::MaskImg)
	{
		//TODO, somehow create this.mask
	}


	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Geometry& otherGeometry) const
	{
		switch (otherGeometry.shapeStyle)
		{
		case ListOfShapes::Spheres:
			return otherGeometry.getDistance(*this);
			break;
		case ListOfShapes::Mesh:
			return otherGeometry.getDistance(*this);
			break;
		case ListOfShapes::MaskImg:
			//TODO
			REPORT("this.MaskImg vs MaskImg is not implemented yet!");
			break;
		default:
			REPORT("TODO: throw new RuntimeException(cannot calculate distance!)");
			return 999.9f;
		}

		return 10.f;
	}


	/** construct AABB from the the mask image considering
	    only non-zero valued voxels, and considering mask's
		 offset and resolution */
	void setAABB(AxisAlignedBoundingBox& AABB) const
	{
		//scan through the image and find extremal coordinates of non-zero voxels
		//TODO
		REPORT("not implemented yet!");
	}
};
#endif
