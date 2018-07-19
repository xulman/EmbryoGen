#ifndef MASKIMG_H
#define MASKIMG_H

#include <i3d/image3d.h>
#include <i3d/DistanceTransform.h>
#include "../util/report.h"
#include "Geometry.h"
#include "Spheres.h"

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

	/** (cached) resolution of the distImg [pixels per micrometer] */
	Vector3d<FLOAT> distImgRes;
	/** (cached) offset of the distImg's "minCorner" [micrometer] */
	Vector3d<FLOAT> distImgOff;
	/** (cached) offset of the distImg's "maxCorner" [micrometer] */
	Vector3d<FLOAT> distImgFarEnd;

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
			getDistanceToSpheres((class Spheres*)&otherGeometry,l);
			break;
		case ListOfShapeForms::Mesh:
			//TODO: attempt to project mesh vertices into the mask image and look for collision
			REPORT("this.MaskImg vs Mesh is not implemented yet!");
			break;
		case ListOfShapeForms::MaskImg:
			REPORT("this.MaskImg vs MaskImg is not implemented yet!");
			//getDistanceToMaskImg((MaskImg*)&otherGeometry,l);
			break;
		default:
			throw new std::runtime_error("Geometry::getDistance(): Not supported combination of shape representations.");
		}
	}

	/** Specialized implementation of getDistance() for MaskImg & Spheres geometries.
	    Rasterizes the 'other' spheres into the 'local' MaskImg and finds min distance
		 for every other sphere. These nearest surface distances and corresponding
		 ProximityPairs are added to the output list l. */
	void getDistanceToSpheres(const class Spheres* otherSpheres,
	                          std::list<ProximityPair>& l) const
	{
		//da plan: determine bounding box within this MaskImg where
		//we can potentially see any piece of the foreign Spheres;
		//sweep it and consider voxel centres; construct a thought
		//vector from a sphere's centre to the voxel centre, make it
		//that sphere's radius long and check the end of this vector
		//(when positioned at sphere's centre) if it falls into the
		//currently examined voxel; if it does, we have found a voxel
		//that contains a piece of the sphere's surface
		//
		//if half of a voxel diagonal is pre-calculated, one can optimize
		//by first comparing it against abs.val. of the difference of
		//the sphere's radius from the length of the thought vector

		//sweeping box in micrometers, initiated to match spheres' bounding box
		Vector3d<FLOAT> minSweep(otherSpheres->AABB.minCorner),
		                maxSweep(otherSpheres->AABB.maxCorner);

		//update the sweeping box
		minSweep.elemMax(distImgOff);  //in mu
		maxSweep.elemMin(distImgFarEnd);

		//convert to pixel distances
		minSweep -= distImgOff;
		maxSweep -= distImgOff;

		minSweep.elemMult(distImgRes); //in px
		maxSweep.elemMult(distImgRes);

		//the sweeping box in pixels, in coordinates of this distImg
		Vector3d<size_t>
		  minSweepPX( (size_t)std::floor(minSweep.x),(size_t)std::floor(minSweep.y),(size_t)std::floor(minSweep.z) ),
		  maxSweepPX( (size_t) std::ceil(maxSweep.x),(size_t) std::ceil(maxSweep.y),(size_t) std::ceil(maxSweep.z) );
		Vector3d<size_t> curPos;

		//a "universal" (and inaccurate) half "thickness" of spheres' surfaces
		//for detection of voxels that coincide with such volumes
		const FLOAT halfPXdiagonal = distImgRes.len() / 2.0f;

		//shortcuts to the otherGeometry's spheres
		const Vector3d<FLOAT>* const centresO = otherSpheres->getCentres();
		const FLOAT* const radiiO             = otherSpheres->getRadii();
		const int io                          = otherSpheres->getNoOfSpheres();

		//remember the smallest distance observed so far for every foreign sphere...
		FLOAT* distances = new FLOAT[io];
		for (int i = 0; i < io; ++i) distances[i] = TOOFAR;
		//...and where it was observed
		Vector3d<size_t>* hints = new Vector3d<size_t>[io];

		//finally, sweep and check intersection with spheres' surfaces
		for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
		for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
		for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
		{
			//we are now visiting voxels where some sphere can be seen
			minSweep.x = (FLOAT)curPos.x / distImgRes.x;
			minSweep.y = (FLOAT)curPos.y / distImgRes.y;
			minSweep.z = (FLOAT)curPos.z / distImgRes.z;
			minSweep += distImgOff;

			//check the current voxel against all spheres
			for (int i = 0; i < io; ++i)
			{
				maxSweep  = minSweep;
				maxSweep -= centresO[i];
				if (std::abs(maxSweep.len() - radiiO[i]) < halfPXdiagonal)
				{
					//hooray, a voxel (nearby) i-th sphere's surface was found,
					//let's inspect the distImg at this position
					const FLOAT dist = distImg.GetVoxel(curPos.x,curPos.y,curPos.z);

					if (dist < distances[i])
					{
						distances[i] = dist;
						hints[i] = curPos;
					}
				}
			} //over all foreign spheres
		} //over all voxels in the sweeping box

		//add ProximityPairs where found some
		for (int i = 0; i < io; ++i)
		if (distances[i] < TOOFAR)
		{
			//found some pair:
			//determine the local gradient at the coinciding voxel
			Vector3d<FLOAT> grad;
			grad.x  = distImg.GetVoxel(hints[i].x+1,hints[i].y,hints[i].z); //TODO: might get outside image!
			grad.x -= distImg.GetVoxel(hints[i].x-1,hints[i].y,hints[i].z);
			grad.y  = distImg.GetVoxel(hints[i].x,hints[i].y+1,hints[i].z);
			grad.y -= distImg.GetVoxel(hints[i].x,hints[i].y-1,hints[i].z);
			grad.z  = distImg.GetVoxel(hints[i].x,hints[i].y,hints[i].z+1);
			grad.z -= distImg.GetVoxel(hints[i].x,hints[i].y,hints[i].z-1);
			grad.elemMult(distImgRes); //account for anisotropy
			grad /= grad.len2();       //normalize

			//determine the exact point on the sphere surface
			Vector3d<FLOAT> exactSurfPoint;
			exactSurfPoint.x = (FLOAT)hints[i].x / distImgRes.x; //coordinate within in the image, in microns
			exactSurfPoint.y = (FLOAT)hints[i].y / distImgRes.y;
			exactSurfPoint.z = (FLOAT)hints[i].z / distImgRes.z;
			exactSurfPoint += distImgOff;  //now real world coordinate of the pixel's centre
			exactSurfPoint -= centresO[i]; //now vector from sphere's centre
			exactSurfPoint *= radiiO[i] / exactSurfPoint.len(); //stretch the vector
			exactSurfPoint += centresO[i]; //now point on the surface...

			//NB: a copy is of the ProximityPair 'p' is created while pushing...
			l.push_back( ProximityPair(exactSurfPoint-grad,exactSurfPoint,
			  distances[i],NULL,(void*)distImg.GetVoxelAddr(curPos.x,curPos.y,curPos.z)) );
		}

		delete[] distances;
		delete[] hints;
	}

	/** Specialized implementation of getDistance() for MaskImg-MaskImg geometries. */
	/*
	void getDistanceToMaskImg(const MaskImg* otherMaskImg,
	                          std::list<ProximityPair>& l) const
	{
	}
	*/


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

	const Vector3d<FLOAT>& getDistImgRes(void) const
	{
		return distImgRes;
	}

	const Vector3d<FLOAT>& getDistImgOff(void) const
	{
		return distImgOff;
	}

	const Vector3d<FLOAT>& getDistImgFarEnd(void) const
	{
		return distImgFarEnd;
	}


	template <class MT>
	void updateWithNewMask(const i3d::Image3d<MT>& _mask)
	{
		//allocates the distance image, voxel values are not initiated
		distImg.CopyMetaData(_mask);
		updateDistImgResOffFarEnd();

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
	/** updates MaskImg::distImgOff, MaskImg::distImgRes and MaskImg::distImgFarEnd
	    to the current MaskImg::distImg */
	void updateDistImgResOffFarEnd(void)
	{
		distImgRes.x = distImg.GetResolution().GetRes().x;
		distImgRes.y = distImg.GetResolution().GetRes().y;
		distImgRes.z = distImg.GetResolution().GetRes().z;

		//"min" corner
		distImgOff.x = distImg.GetOffset().x;
		distImgOff.y = distImg.GetOffset().y;
		distImgOff.z = distImg.GetOffset().z;

		//this mask image's "max" corner in micrometers
		distImgFarEnd.x = (FLOAT)distImg.GetSizeX();
		distImgFarEnd.y = (FLOAT)distImg.GetSizeY();
		distImgFarEnd.z = (FLOAT)distImg.GetSizeZ();
		distImgFarEnd.elemDiv(distImgRes); //in mu
		distImgFarEnd += distImgOff;       //max/far end in mu
	}
};
#endif