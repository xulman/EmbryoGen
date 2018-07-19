#ifndef MASKIMG_H
#define MASKIMG_H

#include <i3d/image3d.h>
#include <i3d/DistanceTransform.h>
#include "../util/report.h"
#include "Geometry.h"

/**
 * Shape form based on image (given its size [px], offset [micrometers] and
 * resolution [px/micrometers]) where non-zero valued voxels represent each
 * a piece of a volume that is taken up by the represented agent. The set of
 * non-zero voxels need not form single connected component.
 *
 * When interacting with other objects, there are three modes available on
 * how to calculate distances:
 *
 * mask: 0001111000  - 1 represents inside the agent, 0 is outside of the agent
 *
 * a)    TT\____/TT  - distance inside is 0 everywhere, increases away from object
 *
 * b)    TT\    /TT  - distance decreases & increases away from boundary of the object
 *          \__/
 *
 * c)    ___    ___  - distance outside is 0 everywhere, decreases towards the centre of the agent
 *          \__/
 *
 * Actually, the distance inside the object is reported with negative sign.
 * This way, an agent can distinguish how far it is from the "edge" (taking
 * absolute value of the distance) and on which "side of the edge" it is.
 *
 * Variant a) defines a shape/agent that does not distinguish within its volume,
 * making your agents "be afraid of non-zero distances" will make them stay
 * inside this geometry. Variant b) will make the agents stay along the boundary/surface
 * of this geometry. Variant c) will prevent agents from staying inside this geometry.
 *
 * The class was designed with the assumption that agents who interact with this
 * agent will want to minimize their mutual (surface) distance.
 *
 * Author: Vladimir Ulman, 2018
 */
class MaskImg: public Geometry
{
public:
	/** Defines how distances will be calculated on this geometry, see class MaskImg docs */
	typedef enum
	{
		ZeroIN_GradOUT=0,
		GradIN_GradOUT=1,
		GradIN_ZeroOUT=2
	} DistanceModel;

private:
	/** Image with precomputed distances, it is of the same offset, size, resolution
	    (see docs of class MaskImg) as the one given during construction of this object */
	i3d::Image3d<float> distImg;

	/** (cached) offset of the distImg [micrometer] */
	Vector3d<FLOAT> distImgOff;
	/** (cached) resolution of the distImg [pixels per micrometer] */
	Vector3d<FLOAT> distImgRes;

	/** This is just a reminder of how the MaskImg::distImg was created, since we don't
	    have reference or copy to the original source image and we cannot reconstruct it
	    easily... */
	const DistanceModel model;

public:
	/** constructor can be a bit more memory expensive if _model is GradIN_GradOUT */
	template <class MT>
	MaskImg(const i3d::Image3d<MT>& _mask, DistanceModel _model)
		: Geometry(ListOfShapeForms::MaskImg), model(_model)
	{
		updateWithNewMask(_mask);
	}

	/** just for debug purposes: save the distance image to a filename */
	void saveDistImg(const char* filename)
	{
		distImg.SaveImage(filename);
	}


	// ------------- distances -------------
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


	// ------------- AABB -------------
	/** construct AABB from the the mask image considering
	    only non-zero valued voxels, and considering mask's
		 offset and resolution */
	void setAABB(AxisAlignedBoundingBox& AABB) const override
	{
		if (model == GradIN_ZeroOUT)
		{
			//check distImg < 0, and take "outer boundary" of voxels for the AABB
			AABB.reset();

			//micrometer [X,Y,Z] coordinates of pixels at [x,y,z]
			float X,Y,Z;

			const float* f = distImg.GetFirstVoxelAddr();
			for (size_t z = 0; z < distImg.GetSizeZ(); ++z)
			for (size_t y = 0; y < distImg.GetSizeY(); ++y)
			for (size_t x = 0; x < distImg.GetSizeX(); ++x)
			{
				if (*f < 0)
				{
					//get micrometers coordinate
					X = x/distImgRes.x + distImgOff.x;
					Y = y/distImgRes.y + distImgOff.y;
					Z = z/distImgRes.z + distImgOff.z;

					//update the AABB
					AABB.minCorner.x = std::min(AABB.minCorner.x, X);
					AABB.maxCorner.x = std::max(AABB.maxCorner.x, X+distImgRes.x);

					AABB.minCorner.y = std::min(AABB.minCorner.y, Y);
					AABB.maxCorner.y = std::max(AABB.maxCorner.y, Y+distImgRes.y);

					AABB.minCorner.z = std::min(AABB.minCorner.z, Z);
					AABB.maxCorner.z = std::max(AABB.maxCorner.z, Z+distImgRes.z);
				}
				++f;
			}
		}
		else
		{
			//the interesting part of the shape is either everywhere in the image
			//(GradIN_GradOUT model), or outside the shape (ZeroIN_GradOUT model),
			//the AABB is therefore the whole image
			AABB.minCorner = distImgOff;

			AABB.maxCorner.x = distImg.GetSizeX() / distImgRes.x; //size in micrometers
			AABB.maxCorner.y = distImg.GetSizeY() / distImgRes.y;
			AABB.maxCorner.z = distImg.GetSizeZ() / distImgRes.z;
			AABB.maxCorner  += distImgOff;
		}
	}


	// ------------- get/set methods -------------
	const i3d::Image3d<float>& getDistImg(void) const
	{
		return distImg;
	}

	const Vector3d<FLOAT>& getDistImgOff(void) const
	{
		return distImgOff;
	}

	const Vector3d<FLOAT>& getDistImgRes(void) const
	{
		return distImgRes;
	}


	template <class MT>
	void updateWithNewMask(const i3d::Image3d<MT>& _mask)
	{
		//allocates the distance image, voxel values are not initiated
		distImg.CopyMetaData(_mask);
		updateDistImgResOff();

		//running pointers (dimension independent code)
		const MT* m = _mask.GetFirstVoxelAddr();
		float*    f = distImg.GetFirstVoxelAddr();
		float* const fE = f + distImg.GetImageSize();

		//do DT inside the mask?
		if (model == GradIN_GradOUT || model == GradIN_ZeroOUT)
		{
			//yes, "extract" non-zero (inside) mask
			while (f != fE)
				*f++ = *m++ > 0 ? 1.0f : 0.0f;
		}
		else
		{
			//ZeroIN_GradOUT, "extract" zero (outside) mask
			while (f != fE)
				*f++ = *m++ > 0 ? 0.0f : 1.0f;
		}

		//distance _transform_ non-zero part
		i3d::FastSaito(distImg, 1.0f, true);

		//if the "inside" was DT'ed, we need to "inverse" the distances
		if (model == GradIN_GradOUT || model == GradIN_ZeroOUT)
		{
			f = distImg.GetFirstVoxelAddr();
			while (f != fE)
				*f++ *= -1.0f;
		}

		if (model == GradIN_GradOUT)
		{
			//we need to additionally do the "GradOUT" part,
			//and we cannot distance _transform_ directly distImg
			i3d::Image3d<float> dtImg;
			dtImg.CopyMetaData(_mask);

			m = _mask.GetFirstVoxelAddr();
			float* ff = dtImg.GetFirstVoxelAddr();
			float* const fEE = ff + dtImg.GetImageSize();

			//"extract" zero (outside) mask
			while (ff != fEE)
				*ff++ = *m++ > 0 ? 0.0f : 1.0f;

			//distance _transform_ non-zero part
			i3d::FastSaito(dtImg, 1.0f, true);

			//now copy non-zero values from dtImg into zero values of distImg
			f = distImg.GetFirstVoxelAddr();
			ff = dtImg.GetFirstVoxelAddr();
			while (f != fE)
			{
				*f = *ff > 0 ? *ff-1 : *f;
				++f; ++ff;
			}
		}
	}


private:
	/** updates MaskImg::distImgOff and MaskImg::distImgRes
	    to the current MaskImg::distImg */
	void updateDistImgResOff(void)
	{
		distImgOff.x = distImg.GetOffset().x;
		distImgOff.y = distImg.GetOffset().y;
		distImgOff.z = distImg.GetOffset().z;

		distImgRes.x = distImg.GetResolution().GetRes().x;
		distImgRes.y = distImg.GetResolution().GetRes().y;
		distImgRes.z = distImg.GetResolution().GetRes().z;
	}
};
#endif
