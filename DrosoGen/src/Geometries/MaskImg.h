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
	std::list<ProximityPair>*
	getDistance(const Geometry& otherGeometry) const
	{
		//default return value
		std::list<ProximityPair>* l = emptyCollisionListPtr;

		switch (otherGeometry.shapeStyle)
		{
		case ListOfShapes::Spheres:
			//find collision "from the other side" and revert orientation of all elements on the list
			l = otherGeometry.getDistance(*this);
			for (auto lit = l->begin(); lit != l->end(); lit++) lit->swap();
			break;
		case ListOfShapes::Mesh:
			//find collision "from the other side" and revert orientation of all elements on the list
			l = otherGeometry.getDistance(*this);
			for (auto lit = l->begin(); lit != l->end(); lit++) lit->swap();
			break;
		case ListOfShapes::MaskImg:
			//TODO identity case
			REPORT("this.MaskImg vs MaskImg is not implemented yet!");
			break;
		default:
			REPORT("TODO: throw new RuntimeException(cannot calculate distance!)");
		}

		return l;
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
