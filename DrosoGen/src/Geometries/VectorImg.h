#ifndef VECTORIMG_H
#define VECTORIMG_H

#include <i3d/image3d.h>
#include <i3d/DistanceTransform.h>
#include "../util/report.h"
#include "../util/FlowField.h"
#include "Geometry.h"
#include "Spheres.h"

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

	/** just for debug purposes: save the vector field.x image to a filename */
	void saveFFx(const char* filename)
	{ X.SaveImage(filename); }

	/** just for debug purposes: save the vector field.y image to a filename */
	void saveFFy(const char* filename)
	{ Y.SaveImage(filename); }

	/** just for debug purposes: save the vector field.z image to a filename */
	void saveFFz(const char* filename)
	{ Z.SaveImage(filename); }

	/** just for debug purposes: save the complete vector field (3) images
	    using 'filename' as a prefix */
	void saveFF(const char* filename)
	{
		char fn[1024];
		sprintf(fn,"%s_x.ics",filename);  X.SaveImage(fn);
		sprintf(fn,"%s_y.ics",filename);  Y.SaveImage(fn);
		sprintf(fn,"%s_z.ics",filename);  Z.SaveImage(fn);
	}

	/** just for debug purposes: save the image with magnitudes of the vectors to a filename */
	void saveFFmagnitudes(const char* filename)
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

		mImg.SaveImage(filename);
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
			REPORT("this.VectorImg vs Mesh is not implemented yet!");
			break;
		case ListOfShapeForms::ScalarImg:
			//find collision "from the other side"
			getSymmetricDistance(otherGeometry,l);
			break;
		case ListOfShapeForms::VectorImg:
			//TODO identity case
			REPORT("this.VectorImg vs VectorImg is not implemented yet!");
			//getDistanceToVectorImg((VectorImg*)&otherGeometry,l);
			break;

		case ListOfShapeForms::undefGeometry:
			REPORT("Ignoring other geometry of type 'undefGeometry'.");
			break;
		default:
			throw ERROR_REPORT("Not supported combination of shape representations.");
		}
	}

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
	                          std::list<ProximityPair>& l) const
	{
		//da plan: determine bounding box within this VectorImg where
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

		//the sweeping box in pixels, in coordinates of this FF
		Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
		sweepBox.exportInPixelCoords(X, minSweepPX,maxSweepPX);

		//(squared) voxel's volume half-diagonal vector and its length
		//(for detection of voxels that coincide with sphere's surface)
		Vector3d<FLOAT> vecPXhd2(0.5f/imgRes.x,0.5f/imgRes.y,0.5f/imgRes.z);
		const FLOAT     lenPXhd = vecPXhd2.len();
		vecPXhd2.elemMult(vecPXhd2); //make it squared

		//shortcuts to the otherGeometry's spheres
		const Vector3d<FLOAT>* const centresO = otherSpheres->getCentres();
		const FLOAT* const radiiO             = otherSpheres->getRadii();
		const int io                          = otherSpheres->getNoOfSpheres();

		//remember the squared lengths observed so far for every foreign sphere...
		FLOAT* lengths2 = new FLOAT[io];
		switch (policy)
		{ //initialize the array
		case minVec:
			for (int i = 0; i < io; ++i) lengths2[i] = TOOFAR;
			break;
		case maxVec:
		case avgVec:
			for (int i = 0; i < io; ++i) lengths2[i] = 0;
			break;
		case allVec:
		default: ;
			//no init required
		}
		//...what was observed...
		Vector3d<FLOAT>* vecs = new Vector3d<FLOAT>[io]; //inits to zero vector by default
		//...and where it was observed
		Vector3d<size_t>* hints = new Vector3d<size_t>[io];

		//holder for the currently examined vectors in the FF
		Vector3d<FLOAT> vec;

		//aux position vectors: current voxel's centre and somewhat sphere's surface point
		Vector3d<FLOAT> centre, surfPoint;

		//finally, sweep and check intersection with spheres' surfaces
		for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
		for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
		for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
		{
			//we are now visiting voxels where some sphere can be seen,
			//get micron coordinate of the current voxel's centre
			centre.x = ((FLOAT)curPos.x +0.5f) / imgRes.x;
			centre.y = ((FLOAT)curPos.y +0.5f) / imgRes.y;
			centre.z = ((FLOAT)curPos.z +0.5f) / imgRes.z;
			centre += imgOff;

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
						//let's inspect the FF's vector at this position
						vec.x = X.GetVoxel(curPos.x,curPos.y,curPos.z);
						vec.y = Y.GetVoxel(curPos.x,curPos.y,curPos.z);
						vec.z = Z.GetVoxel(curPos.x,curPos.y,curPos.z);

						switch (policy)
						{
						case minVec:
							if (vec.len2() < lengths2[i])
							{
								lengths2[i] = vec.len2();
								vecs[i] = vec;
								hints[i] = curPos;
							}
							break;
						case maxVec:
							if (vec.len2() > lengths2[i])
							{
								lengths2[i] = vec.len2();
								vecs[i] = vec;
								hints[i] = curPos;
							}
							break;
						case avgVec:
							vecs[i] += vec;
							lengths2[i] += 1; //act as count here
							break;
						case allVec:
							//exact point on the sphere's surface
							surfPoint  = centre;
							surfPoint -= centresO[i];
							surfPoint *= radiiO[i]/surfPoint.len();
							surfPoint += centresO[i];

							//create immediately the ProximityPair
							l.emplace_back( surfPoint+vec,surfPoint, vec.len(),
							  (signed)X.GetIndex(curPos.x,curPos.y,curPos.z),i );
							break;
						default: ;
						}

					} //if intersecting voxel was found
				} //if nearby voxel was found
			} //over all foreign spheres
		} //over all voxels in the sweeping box

		//process (potentially) the one vector per sphere
		if (policy == minVec)
		{
			//add ProximityPairs where found some
			for (int i = 0; i < io; ++i)
			if (lengths2[i] < TOOFAR)
			{
				//found one
				//calculate exact point on the sphere's surface (using hints[i])
				surfPoint.x = ((FLOAT)hints[i].x +0.5f) / imgRes.x;
				surfPoint.y = ((FLOAT)hints[i].y +0.5f) / imgRes.y;
				surfPoint.z = ((FLOAT)hints[i].z +0.5f) / imgRes.z;
				surfPoint += imgOff;
				surfPoint -= centresO[i];
				surfPoint *= radiiO[i]/surfPoint.len();
				surfPoint += centresO[i];

				//this is from VectorImg perspective (local = VectorImg, other = Sphere),
				//it reports index of the relevant foreign sphere
				l.emplace_back( surfPoint+vecs[i],surfPoint, std::sqrt(lengths2[i]),
				  (signed)X.GetIndex(hints[i].x,hints[i].y,hints[i].z),i );
			}
		}
		else if (policy == maxVec)
		{
			//add ProximityPairs where found some
			for (int i = 0; i < io; ++i)
			if (lengths2[i] > 0)
			{
				//found one
				//calculate exact point on the sphere's surface (using hints[i])
				surfPoint.x = ((FLOAT)hints[i].x +0.5f) / imgRes.x;
				surfPoint.y = ((FLOAT)hints[i].y +0.5f) / imgRes.y;
				surfPoint.z = ((FLOAT)hints[i].z +0.5f) / imgRes.z;
				surfPoint += imgOff;
				surfPoint -= centresO[i];
				surfPoint *= radiiO[i]/surfPoint.len();
				surfPoint += centresO[i];

				//this is from VectorImg perspective (local = VectorImg, other = Sphere),
				//it reports index of the relevant foreign sphere
				l.emplace_back( surfPoint+vecs[i],surfPoint, std::sqrt(lengths2[i]),
				  (signed)X.GetIndex(hints[i].x,hints[i].y,hints[i].z),i );
			}
		}
		else if (policy == avgVec)
		{
			//add ProximityPairs where found some
			for (int i = 0; i < io; ++i)
			if (lengths2[i] > 0)
			{
				//found at least one
				vecs[i] /= lengths2[i];

				//this is from VectorImg perspective (local = VectorImg, other = Sphere),
				//it reports index of the relevant foreign sphere
				l.emplace_back( centresO[i]+vecs[i],centresO[i], vecs[i].len(), 0,i );
				//NB: average vector is no particular existing vector, hence local hint = 0,
				//    and placed in the sphere's centre
			}
		}
		//else allVec -- this has been already done above

		delete[] lengths2;
		delete[] vecs;
		delete[] hints;
	}

	/** Specialized implementation of getDistance() for VectorImg-VectorImg geometries. */
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
		//the AABB is the whole image
		AABB.minCorner = imgOff;
		AABB.maxCorner = imgFarEnd;
	}


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
		imgRes.x = X.GetResolution().GetRes().x;
		imgRes.y = X.GetResolution().GetRes().y;
		imgRes.z = X.GetResolution().GetRes().z;

		//"min" corner
		imgOff.x = X.GetOffset().x;
		imgOff.y = X.GetOffset().y;
		imgOff.z = X.GetOffset().z;

		//this mask image's "max" corner in micrometers
		imgFarEnd.x = (FLOAT)X.GetSizeX();
		imgFarEnd.y = (FLOAT)X.GetSizeY();
		imgFarEnd.z = (FLOAT)X.GetSizeZ();
		imgFarEnd.elemDivBy(imgRes); //in mu
		imgFarEnd += imgOff;         //max/far end in mu
	}
};
#endif
