#ifndef MASKIMG_H
#define MASKIMG_H

#include <i3d/image3d.h>
#include "../report.h"
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
	MaskImg(void): Geometry(ListOfShapeForms::MaskImg)
	{
		//TODO, somehow create this.mask
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
			//find collision "from the other side" and revert orientation of all elements on the list
			l = otherGeometry.getDistance(*this);
			for (auto lit = l->begin(); lit != l->end(); lit++) lit->swap();
			break;
		case ListOfShapeForms::Mesh:
			//find collision "from the other side" and revert orientation of all elements on the list
			l = otherGeometry.getDistance(*this);
			for (auto lit = l->begin(); lit != l->end(); lit++) lit->swap();
			break;
		case ListOfShapeForms::MaskImg:
			//TODO identity case
			REPORT("this.MaskImg vs MaskImg is not implemented yet!");
			break;
		default:
			throw new std::runtime_error("Geometry::getDistance(): Not supported combination of shape representations.");
		}

		return l;
	}


	/** construct AABB from the the mask image considering
	    only non-zero valued voxels, and considering mask's
		 offset and resolution */
	void setAABB(AxisAlignedBoundingBox& AABB) const override
	{
		//scan through the image and find extremal coordinates of non-zero voxels
		//TODO
		REPORT("not implemented yet!");
	}
};
#endif
