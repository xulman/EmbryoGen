#ifndef MASKIMG_H
#define MASKIMG_H

#include <i3d/image3d.h>
#include "../util/report.h"
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


	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const override
	{
		switch (otherGeometry.shapeForm)
		{
		case ListOfShapeForms::Spheres:
			//TODO: attempt to rasterize Spheres within their AABB and look for collision
			REPORT("this.MaskImg vs Spheres is not implemented yet!");
			break;
		case ListOfShapeForms::Mesh:
			//TODO: attempt to project mesh vertices into the mask image and look for collision
			REPORT("this.MaskImg vs Mesh is not implemented yet!");
			break;
		case ListOfShapeForms::MaskImg:
			//TODO identity case
			REPORT("this.MaskImg vs MaskImg is not implemented yet!");
			break;
		default:
			throw new std::runtime_error("Geometry::getDistance(): Not supported combination of shape representations.");
		}
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
