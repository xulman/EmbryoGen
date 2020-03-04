#ifndef GEOMETRY_VECTORIMG_H
#define GEOMETRY_VECTORIMG_H

#include <i3d/image3d.h>
#include "../util/report.h"
#include "../util/FlowField.h"
#include "Geometry.h"
class Spheres;

/**
 * The class essentially represents a 3D vector field that spans a certain volume.
 *
 * The class was designed with the assumption that agents who interact with this
 * agent will want to learn about vectors at their surface. So, during construction
 * or later, the caller can decide what policy to apply during Geometry::getDistance()
 * calls. The policies to choose from can be, e.g., return min/max, average or all
 * vectors that exists at the surface of the other agent. The vectors are reported
 * in a form of ProximityPairs. If other policy is demanded, one does not need to
 * call getDistance() at all (or inherit from this class overriding getDistance()
 * with no-code-function) and can directly inspect the stored vectors.
 *
 * This class is, in fact, an alternative to the FlowField<FLOAT>, they both
 * represent x,y,z elements of a spatial distribution of 3D real vectors.
 *
 * Author: Vladimir Ulman, 2018
 */
class VectorImg: public Geometry
{
public:
	/** Defines how distances will be calculated on this geometry, see class VectorImg docs */
	typedef enum
	{
		minVec=0,
		maxVec=1,
		avgVec=2,
		allVec=3
	} ChoosingPolicy;

private:
	/** Images that together represent the 3D vector field, aka FF.
	    They are of the same offset, size, resolution as the sample
	    one given during construction of this object. */
	i3d::Image3d<FLOAT> X,Y,Z;

	/** (cached) resolution of the FF [pixels per micrometer] */
	Vector3d<FLOAT> imgRes;
	/** (cached) offset of the FF's "minCorner" [micrometer] */
	Vector3d<FLOAT> imgOff;
	/** (cached) offset of the FF's "maxCorner" [micrometer] */
	Vector3d<FLOAT> imgFarEnd;

	/** This is the chosen policy for VectorImg::getDistance() */
	const ChoosingPolicy policy;

public:
	/** constructor to hold FF in images similar to the _template */
	template <class T>
	VectorImg(const i3d::Image3d<T>& _template, ChoosingPolicy _policy)
		: Geometry(ListOfShapeForms::VectorImg), policy(_policy)
	{
		X.CopyMetaData(_template);
		Y.CopyMetaData(_template);
		Z.CopyMetaData(_template);

		zeroAll();
		updateResOffFarEnd();
	}

	/** copy constructor */
	/* not necessary as long as no new() constructs are used in the main c'tor
	VectorImg(const VectorImg& s)
		: Geometry(ListOfShapeForms::VectorImg), policy(s.getChoosingPolicy())
	{
		REPORT("copy c'tor");
		X         = s.getImgX();
		Y         = s.getImgY();
		Z         = s.getImgZ();
		imgRes    = s.getImgRes();
		imgOff    = s.getImgOff();
		imgFarEnd = s.getImgFarEnd();
	}
	*/

	/** inserts zero vectors to the vector field */
	void zeroAll(void)
	{
		X.GetVoxelData() = 0;
		Y.GetVoxelData() = 0;
		Z.GetVoxelData() = 0;
	}

	/** just for debug purposes: save the vector field.x image to a fullFilename */
	void saveFFx(const char* fullFilename)
	{ X.SaveImage(fullFilename); }

	/** just for debug purposes: save the vector field.y image to a fullFilename */
	void saveFFy(const char* fullFilename)
	{ Y.SaveImage(fullFilename); }

	/** just for debug purposes: save the vector field.z image to a fullFilename */
	void saveFFz(const char* fullFilename)
	{ Z.SaveImage(fullFilename); }

	/** just for debug purposes: save the complete vector field (3) images
	    using 'filenamesPrefix' as a prefix */
	void saveFF(const char* filenamesPrefix)
	{
		char fn[1024];
		sprintf(fn,"%s_x.ics",filenamesPrefix);  X.SaveImage(fn);
		sprintf(fn,"%s_y.ics",filenamesPrefix);  Y.SaveImage(fn);
		sprintf(fn,"%s_z.ics",filenamesPrefix);  Z.SaveImage(fn);
	}

	/** just for debug purposes: save the image with magnitudes of the vectors to a filename */
	void saveFFmagnitudes(const char* fullFilename)
	{
		//create a new image of the same dimension
		i3d::Image3d<float> mImg;
		mImg.CopyMetaData(X);

		//running pointers...
		const FLOAT* x = X.GetFirstVoxelAddr();     //input vector elements
		const FLOAT* y = Y.GetFirstVoxelAddr();
		const FLOAT* z = Z.GetFirstVoxelAddr();
		float* m = mImg.GetFirstVoxelAddr();        //for vector magnitude
		float* const mE = m + mImg.GetImageSize();

		//sweep the output image
		while (m != mE)
		{
			*m = std::sqrt( (*x * *x) + (*y * *y) + (*z * *z) );
			++m; ++x; ++y; ++z;
		}

		mImg.SaveImage(fullFilename);
	}


	// ------------- distances -------------
	/** calculate min surface distance between myself and some foreign agent */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const override;

	/** Specialized implementation of getDistance() for VectorImg & Spheres geometries.
	    Rasterizes the 'other' spheres into the 'local' VectorImg and, based on this->policy
	    finds the right distance for every other sphere. These surface distances and
	    corresponding ProximityPairs are added to the output list l.

	    Every ProximityPair returned here is in fact encoding some vector from the VectorImg,
	    it is no real distance to anything. The 'other' position in the ProximityPair is
	    where the other surface is hitting this VectorImg, and displacement from 'other'
	    to 'local' position in the ProximityPair is the vector itself. It is assumed that
	    the other interacting party knows well how to interpret it.

	    One could say that the ProximityPairs are hijacked in this special type of "geometry",
	    and indeed it is true. The reason for it is that the only interaction between agents
	    is allowed (currently) via ShadowAgent::geometry attribute, which supports communication
	    by means of ProximityPairs (via Geometry::getDistance()). Nevertheless, one is always
	    allowed to read out the ShadowAgent::geometry directly and process it according to
	    one's own needs (in this light this function is more a convenience function than
	    anything else). */
	void getDistanceToSpheres(const class Spheres* otherSpheres,
	                          std::list<ProximityPair>& l) const;

	/** Specialized implementation of getDistance() for VectorImg-VectorImg geometries. */
	/*
	void getDistanceToVectorImg(const VectorImg* otherVectorImg,
	                            std::list<ProximityPair>& l) const;
	*/


	// ------------- AABB -------------
	/** construct AABB from the the mask image considering
	    only non-zero valued voxels, and considering mask's
	    offset and resolution */
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const override;


	// ------------- get/set methods -------------
	Vector3d<FLOAT> getVector(const size_t x,const size_t y,const size_t z) const
	{
		const size_t index = X.GetIndex(x,y,z);
		return Vector3d<FLOAT>(X.GetVoxel(index), Y.GetVoxel(index), Z.GetVoxel(index));
	}

	void getVector(const size_t x,const size_t y,const size_t z, //input
	               Vector3d<FLOAT>& v) const                     //output
	{
		const size_t index = X.GetIndex(x,y,z);
		v.x = X.GetVoxel(index);
		v.y = Y.GetVoxel(index);
		v.z = Z.GetVoxel(index);
	}

	void setVector(const size_t x,const size_t y,const size_t z, //input
	               const Vector3d<FLOAT>& v)                     //input
	{
		const size_t index = X.GetIndex(x,y,z);
		X.SetVoxel(index,v.x);
		Y.SetVoxel(index,v.y);
		Z.SetVoxel(index,v.z);
	}

	void proxifyFF(FlowField<FLOAT>& FF)
	{
		FF.proxify(&X,&Y,&Z);
		DEBUG_REPORT("The FF.isConsistent() gives " << (FF.isConsistent() ? "true" : "false"));
	}

	const i3d::Image3d<FLOAT>& getImgX(void) const
	{ return X; }
	const i3d::Image3d<FLOAT>& getImgY(void) const
	{ return Y; }
	const i3d::Image3d<FLOAT>& getImgZ(void) const
	{ return Z; }

	const Vector3d<FLOAT>& getImgRes(void) const
	{
		return imgRes;
	}

	const Vector3d<FLOAT>& getImgOff(void) const
	{
		return imgOff;
	}

	const Vector3d<FLOAT>& getImgFarEnd(void) const
	{
		return imgFarEnd;
	}

	ChoosingPolicy getChoosingPolicy(void) const
	{
		return policy;
	}


private:
	/** updates VectorImg::imgOff, VectorImg::imgRes and VectorImg::imgFarEnd
	    to the current VectorImg::x */
	void updateResOffFarEnd(void)
	{
		imgRes.fromI3dVector3d( X.GetResolution().GetRes() );

		//"min" corner
		imgOff.fromI3dVector3d( X.GetOffset() );

		//this mask image's "max" corner in micrometers
		imgFarEnd.from( Vector3d<size_t>(X.GetSize()) )
		         .toMicrons(imgRes,imgOff);
	}

	// ----------------- support for serialization and deserealization -----------------
public:
	long getSizeInBytes() const override;
};
#endif
