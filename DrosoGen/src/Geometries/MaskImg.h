#ifndef MASKIMG_H
#define MASKIMG_H

#include "Geometry.h"

template <class MT>
class MaskImg: public Geometry
{
private:
	Image3d<MT> mask;

public:
	MaskImg(void): shapeStyle(MaskImg)
	{
		//TODO, somehow create this.mask
	}

	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Spheres& spheres) const
	{
		return spheres.getDistance(*this);
	}

	/** calculate min distance between myself and some foreign agent */
	FLOAT getDistance(const Mesh& mesh) const
	{
		return mesh.getDistance(*this);
	}

	/** calculate min distance between myself and some foreign agent */
	template <class FMT> //Foreign MT
	FLOAT getDistance(const MaskImg<FMT>& mask) const
	{
		//TODO
		REPORT("this.MaskImg vs MaskImg is not implemented yet!");
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
}
#endif
