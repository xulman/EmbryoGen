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
 * The class was originally designed with the assumption that agents who interact
 * with this agent will want to minimize their mutual (surface) distance.
 *
 * However, the use of this class need not to be restricted to this
 * use-case -- in which case the best is to create a new class that
 * inherits from this one, overrides the getDistance() method, and
 * declares shapeForm = Geometry::ListOfShapeForms::undefGeometry.
 *
 * Author: Vladimir Ulman, 2018
 */
class ScalarImg: public Geometry
{
public:
	/** Defines how distances will be calculated on this geometry, see class ScalarImg docs */
	typedef enum
	{
		ZeroIN_GradOUT=0,
		GradIN_GradOUT=1,
		GradIN_ZeroOUT=2
	} DistanceModel;

private:
	/** Image with precomputed distances, it is of the same offset, size, resolution
	    (see docs of class ScalarImg) as the one given during construction of this object */
	i3d::Image3d<float> distImg;

	/** (cached) resolution of the distImg [pixels per micrometer] */
	Vector3d<FLOAT> distImgRes;
	/** (cached) offset of the distImg's "minCorner" [micrometer] */
	Vector3d<FLOAT> distImgOff;
	/** (cached) offset of the distImg's "maxCorner" [micrometer] */
	Vector3d<FLOAT> distImgFarEnd;

	/** This is just a reminder of how the ScalarImg::distImg was created, since we don't
	    have reference or copy to the original source image and we cannot reconstruct it
	    easily... */
	const DistanceModel model;

public:
	/** constructor can be a bit more memory expensive if _model is GradIN_GradOUT */
	template <class MT>
	ScalarImg(const i3d::Image3d<MT>& _mask, DistanceModel _model)
		: Geometry(ListOfShapeForms::ScalarImg), model(_model)
	{
		updateWithNewMask(_mask);
	}

	/** copy constructor */
	/* not necessary as long as no new() constructs are used in the main c'tor
	ScalarImg(const ScalarImg& s)
		: Geometry(ListOfShapeForms::ScalarImg), model(s.getDistImgModel())
	{
		REPORT("copy c'tor");
		distImg       = s.getDistImg();
		distImgRes    = s.getDistImgRes();
		distImgOff    = s.getDistImgOff();
		distImgFarEnd = s.getDistImgFarEnd();
	}
	*/

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
			REPORT("this.ScalarImg vs Mesh is not implemented yet!");
			break;
		case ListOfShapeForms::ScalarImg:
			//TODO identity case
			REPORT("this.ScalarImg vs ScalarImg is not implemented yet!");
			//getDistanceToScalarImg((ScalarImg*)&otherGeometry,l);
			break;
		case ListOfShapeForms::VectorImg:
			REPORT("this.ScalarImg vs VectorImg is not implemented yet!");
			//getDistanceToVectorImg((VectorImg*)&otherGeometry,l);
			break;

		case ListOfShapeForms::undefGeometry:
			REPORT("Ignoring other geometry of type 'undefGeometry'.");
			break;
		default:
			throw ERROR_REPORT("Not supported combination of shape representations.");
		}
	}

	/** Specialized implementation of getDistance() for ScalarImg & Spheres geometries.
	    Rasterizes the 'other' spheres into the 'local' ScalarImg and finds min distance
	    for every other sphere. These nearest surface distances and corresponding
	    ProximityPairs are added to the output list l.

	    If a Sphere is calculating distance to a ScalarImg, the ProximityPair "points"
	    (the vector from ProximityPair::localPos to ProximityPair::otherPos) from Sphere's
	    surface in the direction parallel to the gradient (determined at 'localPos' in
	    the ScalarImg::distImg) towards a surface of the ScalarImg. The surface, at 'otherPos',
	    is not necessarily accurate as it is estimated from the distance and (local!)
	    gradient only. The length of such a "vector" is the same as a distance to the surface
	    found at 'localPos' in the 'distImg'. If a ScalarImg is calculating distance to a Sphere,
	    the opposite "vector" is created and placed such that the tip of this "vector" points
	    at Sphere's surface. In other words, the tip becomes the base and vice versa. */
	void getDistanceToSpheres(const class Spheres* otherSpheres,
	                          std::list<ProximityPair>& l) const
	{
		//da plan: determine bounding box within this ScalarImg where
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
		AxisAlignedBoundingBox sweepBox(otherSpheres->AABB);

		//intersect the sweeping box with this AABB
		//(as only this AABB wraps around interesting information in the mask image,
		// the image might be larger (especially in the GradIN_ZeroOUT model))
		sweepBox.minCorner.elemMax(AABB.minCorner); //in mu
		sweepBox.maxCorner.elemMin(AABB.maxCorner);

		//the sweeping box in pixels, in coordinates of this distImg
		Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
		sweepBox.exportInPixelCoords(distImg, minSweepPX,maxSweepPX);

		//(squared) voxel's volume half-diagonal vector and its length
		//(for detection of voxels that coincide with sphere's surface)
		Vector3d<FLOAT> vecPXhd2(0.5f/distImgRes.x,0.5f/distImgRes.y,0.5f/distImgRes.z);
		const FLOAT     lenPXhd = vecPXhd2.len();
		vecPXhd2.elemMult(vecPXhd2); //make it squared

		//shortcuts to the otherGeometry's spheres
		const Vector3d<FLOAT>* const centresO = otherSpheres->getCentres();
		const FLOAT* const radiiO             = otherSpheres->getRadii();
		const int io                          = otherSpheres->getNoOfSpheres();

		//remember the smallest distance observed so far for every foreign sphere...
		FLOAT* distances = new FLOAT[io];
		for (int i = 0; i < io; ++i) distances[i] = TOOFAR;
		//...and where it was observed
		Vector3d<size_t>* hints = new Vector3d<size_t>[io];

		//aux position vectors: current voxel's centre and somewhat sphere's surface point
		Vector3d<FLOAT> centre, surfPoint;

		//finally, sweep and check intersection with spheres' surfaces
		for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
		for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
		for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
		{
			//we are now visiting voxels where some sphere can be seen,
			//get micron coordinate of the current voxel's centre
			centre.toMicronsFrom(curPos, distImgRes,distImgOff);

			//check the current voxel against all spheres
			for (int i = 0; i < io; ++i)
			{
				surfPoint  = centre;
				surfPoint -= centresO[i];
				//if sphere's surface would be 2*lenPXhd thick, would the voxel's center be in?
				if (std::abs(surfPoint.len() - radiiO[i]) < lenPXhd)
				{
					//found a voxel _nearby_ i-th sphere's true surface,
					//is there really an intersection?

					//a vector pointing from voxel's centre to the nearest sphere surface
					surfPoint *= radiiO[i]/surfPoint.len() -1.0f;
					//NB: surfPoint *= (radiiO[i] - surfPoint.len()) / surfPoint.len()

					//max (squared; only to avoid using std::abs()) distance to the voxel's centre
					//to see if surfPoint's tip is within this voxel
					surfPoint.elemMult(surfPoint);
					if (surfPoint.x <= vecPXhd2.x && surfPoint.y <= vecPXhd2.y && surfPoint.z <= vecPXhd2.z)
					{
						//hooray, a voxel whose volume is intersecting with i-th sphere's surface
						//let's inspect the distImg at this position
						const FLOAT dist = distImg.GetVoxel(curPos.x,curPos.y,curPos.z);

						if (dist < distances[i])
						{
							distances[i] = dist;
							hints[i] = curPos;
						}
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

			//default value at the coinciding voxel to be used whenever we cannot retrieve proper value
			const FLOAT defValue = distImg.GetVoxel(hints[i].x,hints[i].y,hints[i].z);

			//distance between voxels that occur in the difference calculation
			char span = 2;
			if (hints[i].x+1 < distImg.GetSizeX())
				grad.x  = distImg.GetVoxel(hints[i].x+1,hints[i].y,hints[i].z);
			else
				grad.x  = defValue, span--;
			if (hints[i].x > 0)
				grad.x -= distImg.GetVoxel(hints[i].x-1,hints[i].y,hints[i].z);
			else
				grad.x -= defValue, span--;
			grad.x *= (float)(3-span); //missing /2.0
			//NB: this stretches the value difference as if central difference is calculated,
			//    and leaves it at zero when GetSizeX() == 1 because grad.x == defValue - defValue

			span = 2;
			if (hints[i].y+1 < distImg.GetSizeY())
				grad.y  = distImg.GetVoxel(hints[i].x,hints[i].y+1,hints[i].z);
			else
				grad.y  = defValue, span--;
			if (hints[i].y > 0)
				grad.y -= distImg.GetVoxel(hints[i].x,hints[i].y-1,hints[i].z);
			else
				grad.y -= defValue, span--;
			grad.y *= (float)(3-span); //missing /2.0

			span = 2;
			if (hints[i].z+1 < distImg.GetSizeZ())
				grad.z  = distImg.GetVoxel(hints[i].x,hints[i].y,hints[i].z+1);
			else
				grad.z  = defValue, span--;
			if (hints[i].z > 0)
				grad.z -= distImg.GetVoxel(hints[i].x,hints[i].y,hints[i].z-1);
			else
				grad.z -= defValue, span--;
			grad.z *= (float)(3-span); //missing /2.0

			grad.elemMult(distImgRes);               //account for anisotropy [1/px -> 1/um]
			//DEBUG_REPORT("grad: " << grad << " @ voxel: " << hints[i] << ", distance: " << distances[i]);

			grad.changeToUnitOrZero();               //normalize if not zero vector already
			grad *= -distances[i];                   //extend to the distance (might flip grad!)
			//NB: grad now points always away towards the collision surface

			//determine the exact point on the sphere surface, use again the voxel's centre
			surfPoint.toMicronsFrom(hints[i], distImgRes,distImgOff); //now real world coordinate of the pixel's centre
			surfPoint -= centresO[i]; //now vector from sphere's centre
			surfPoint *= radiiO[i] / surfPoint.len(); //stretch the vector
			surfPoint += centresO[i]; //now point on the surface...

			//this is from ScalarImg perspective (local = ScalarImg, other = Sphere),
			//it reports index of the relevant foreign sphere
			l.emplace_back( surfPoint+grad,surfPoint,
			  distances[i], (signed)distImg.GetIndex(hints[i].x,hints[i].y,hints[i].z),i );
		}

		delete[] distances;
		delete[] hints;
	}

	/** Specialized implementation of getDistance() for ScalarImg-ScalarImg geometries. */
	/*
	void getDistanceToScalarImg(const ScalarImg* otherScalarImg,
	                            std::list<ProximityPair>& l) const
	{
	}
	*/

	/** Specialized implementation of getDistance() for ScalarImg-VectorImg geometries. */
	/*
	void getDistanceToVectorImg(const VectorImg* otherVectorImg,
	                            std::list<ProximityPair>& l) const
	{
	}
	*/


	// ------------- AABB -------------
	/** construct AABB from the the mask image considering
	    only non-zero valued voxels, and considering mask's
	    offset and resolution */
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override
	{
		if (model == GradIN_ZeroOUT)
		{
			//check distImg < 0, and take "outer boundary" of voxels for the AABB
			AABB.reset();

			//micrometer [X,Y,Z] coordinates of pixels at [x,y,z]
			Vector3d<FLOAT> umPos;

			//micrometer size of one voxel
			const Vector3d<FLOAT> oneVxSize( Vector3d<FLOAT>(1).elemDivBy(distImgRes) );

			Vector3d<size_t> pxPos;
			const float* f = distImg.GetFirstVoxelAddr();
			for (pxPos.z = 0; pxPos.z < distImg.GetSizeZ(); ++pxPos.z)
			for (pxPos.y = 0; pxPos.y < distImg.GetSizeY(); ++pxPos.y)
			for (pxPos.x = 0; pxPos.x < distImg.GetSizeX(); ++pxPos.x)
			{
				if (*f < 0)
				{
					//get micrometers coordinate
					umPos.from(pxPos).toMicrons(distImgRes,distImgOff);

					//update the AABB
					AABB.minCorner.elemMin(umPos);

					umPos += oneVxSize;
					AABB.maxCorner.elemMax(umPos);
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
			AABB.maxCorner = distImgFarEnd;
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

	DistanceModel getDistImgModel(void) const
	{
		return model;
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
	/** updates ScalarImg::distImgOff, ScalarImg::distImgRes and ScalarImg::distImgFarEnd
	    to the current ScalarImg::distImg */
	void updateDistImgResOffFarEnd(void)
	{
		distImgRes.fromI3dVector3d( distImg.GetResolution().GetRes() );

		//"min" corner
		distImgOff.fromI3dVector3d( distImg.GetOffset() );

		//this mask image's "max" corner in micrometers
		distImgFarEnd.from( Vector3d<size_t>(distImg.GetSize()) )
		             .toMicrons(distImgRes,distImgOff);
	}
};
#endif
