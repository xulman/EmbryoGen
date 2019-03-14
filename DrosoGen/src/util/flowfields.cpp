/**********************************************************************
*
* flowfields.cpp
*
* This file is part of MitoGen
*
* Copyright (C) 2013-2016 -- Centre for Biomedical Image Analysis (CBIA)
* http://cbia.fi.muni.cz/
*
* MitoGen is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* MitoGen is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with MitoGen. If not, see <http://www.gnu.org/licenses/>.
*
* Author: David Svoboda and Vladimir Ulman
*
* Description: Definition of basic datatypes.
*
***********************************************************************/


#ifdef MITOGEN_DEBUG
  #include <sys/time.h>
#endif
#include <i3d/filters.h>
#include "flowfields.h"

#define ERROR_REPORT(x)  new std::runtime_error( std::string(__FUNCTION__).append("(): ").append(x) )

template <class VT, class FT>
void ImageForwardTransformation(i3d::Image3d<VT> const &srcImg,
				i3d::Image3d<VT> &dstImg,
				FlowField<FT> const &FF)
{
	//sanitary checks...
	if (!FF.isConsistent())
		throw ERROR_REPORT("Flow field is not consistent.");
	if (srcImg.GetSize() != FF.x->GetSize())
		throw ERROR_REPORT("Sizes of input image and flow field do not match!");
	if (srcImg.GetResolution().GetRes() != FF.x->GetResolution().GetRes())
		throw ERROR_REPORT("Resolutions of input image and flow field do not match!");
	if (srcImg.GetOffset() != FF.x->GetOffset())
		throw ERROR_REPORT("Offsets of input image and flow field do not match!");

	//format output image
	dstImg.CopyMetaData(srcImg);
	dstImg.MakeRoom(srcImg.GetSize());
	dstImg.GetVoxelData()=0;

	//short-cuts to the data
	const FT *xFF=FF.x->GetFirstVoxelAddr();
	const FT *yFF=FF.y->GetFirstVoxelAddr();
	const FT *zFF=(FF.z)? FF.z->GetFirstVoxelAddr() : NULL;
	const VT *g=srcImg.GetFirstVoxelAddr();

	//time-savers :-)
	const float xRes=srcImg.GetResolution().GetX();
	const float yRes=srcImg.GetResolution().GetY();
	const float zRes=srcImg.GetResolution().GetZ();

	if (!zFF) {
		//no z-elements in the flow fields, 2D transformation
		DEBUG_REPORT("2D transformation");

		//iterate over the input image and move voxels...
		for (size_t z=0; z < srcImg.GetSizeZ(); ++z)
		  for (size_t y=0; y < srcImg.GetSizeY(); ++y)
		    for (size_t x=0; x < srcImg.GetSizeX(); ++x) {
			//it holds: *g == srcImg.GetVoxel(x,y,z)
			PutPixel(dstImg,(float)x+ *xFF*xRes,
					(float)y+ *yFF*yRes,
					(float)z, *g);
			++xFF;
			++yFF;
			++g;
		    }
	} else {
		//full 3D case...
		DEBUG_REPORT("3D transformation");

		//iterate over the input image and move voxels...
		for (size_t z=0; z < srcImg.GetSizeZ(); ++z)
		  for (size_t y=0; y < srcImg.GetSizeY(); ++y)
		    for (size_t x=0; x < srcImg.GetSizeX(); ++x) {
			//it holds: *g == srcImg.GetVoxel(x,y,z)
			PutPixel(dstImg,(float)x+ *xFF*xRes,
					(float)y+ *yFF*yRes,
					(float)z+ *zFF*zRes, *g);
			/*
			if ((x==46) && (y==33)) std::cout << z << ": "
			  << "+ (" << *xFF*xRes << "," << *yFF*yRes << ","
			  << *zFF*zRes << "), val " << (int)*g << "\n";
			*/
			++xFF;
			++yFF;
			++zFF;
			++g;
		    }
	}
}


template <class FT>
void ConcatenateFlowFields(FlowField<FT> const &srcFF,
			   FlowField<FT> const &appFF,
			   FlowField<FT> &resFF)
{
	//makes a copy of srcFF into the resFF
	//then it calls ConcatenateFlowFields(resFF,appFF);

	//but first, sanitary checks
	if (!srcFF.isConsistent())
		throw ERROR_REPORT("First motion flow field is not consistent.");
	if (!appFF.isConsistent())
		throw ERROR_REPORT("Appended motion flow field is not consistent.");
	if (srcFF.x->GetSize() != appFF.x->GetSize())
		throw ERROR_REPORT("Sizes of input flow fields do not match!");
	if (srcFF.x->GetResolution().GetRes() != appFF.x->GetResolution().GetRes())
		throw ERROR_REPORT("Resolutions of input flow fields do not match!");
	if (srcFF.x->GetOffset() != appFF.x->GetOffset())
		throw ERROR_REPORT("Offsets of input flow fields do not match!");

	//creates brand new FF or just copies the content from srcFF
	//note: owing to the consistency checks, the *srcFF.x and *srcFF.y exists now
	if (!resFF.x) resFF.x=new i3d::Image3d<FT>(*srcFF.x); else *resFF.x=*srcFF.x;
	if (!resFF.y) resFF.y=new i3d::Image3d<FT>(*srcFF.y); else *resFF.y=*srcFF.y;
	if (srcFF.z) {
		//3D case
		if (!resFF.z) resFF.z=new i3d::Image3d<FT>(*srcFF.z); else *resFF.z=*srcFF.z;
	} else {
		//2D case
		if (resFF.z) {
			delete resFF.z;
			resFF.z=NULL;
		}
	}

	ConcatenateFlowFields(resFF,appFF);
}


template <class FT>
void ConcatenateFlowFields(FlowField<FT> &srcFF,
			   FlowField<FT> const &appFF)
{
	if (!srcFF.isConsistent())
		throw ERROR_REPORT("First motion flow field is not consistent.");
	if (!appFF.isConsistent())
		throw ERROR_REPORT("Appended motion flow field is not consistent.");
	if (srcFF.x->GetSize() != appFF.x->GetSize())
		throw ERROR_REPORT("Sizes of input flow fields do not match!");
	if (srcFF.x->GetResolution().GetRes() != appFF.x->GetResolution().GetRes())
		throw ERROR_REPORT("Resolutions of input flow fields do not match!");
	if (srcFF.x->GetOffset() != appFF.x->GetOffset())
		throw ERROR_REPORT("Offsets of input flow fields do not match!");

	DEBUG_REPORT("concatenating flow fields at " << srcFF.x->GetOffset() << " of size " \
	       << i3d::PixelsToMicrons(srcFF.x->GetSize(),srcFF.x->GetResolution())  << " in microns");
	
	//short-cuts to the data
	FT *vx=srcFF.x->GetFirstVoxelAddr();
	FT *vy=srcFF.y->GetFirstVoxelAddr();
	FT *vz=(srcFF.z)? srcFF.z->GetFirstVoxelAddr() : NULL;

	//time-savers :-)
	const float xRes=srcFF.x->GetResolution().GetX();
	const float yRes=srcFF.x->GetResolution().GetY();
	const float zRes=srcFF.x->GetResolution().GetZ();

#ifdef MITOGEN_DEBUG
	//time-measurement
	struct timeval t1,t2;
	float T1,T2;
	gettimeofday(&t1,NULL);
	//TODO VLADO optimize memory access
#endif
	if (!vz) {
		//no z-elements in the flow fields, 2D transformation
		//owing to the consistency checks, the flow fields are 2D images
		DEBUG_REPORT("2D concatenation");

		//iterate over the input flow field and change vectors...
		  for (size_t y=0; y < srcFF.x->GetSizeY(); ++y)
		    for (size_t x=0; x < srcFF.x->GetSizeX(); ++x) {
			//it holds: *vx == srcFF.x->GetVoxel(x,y,0)

			//position after the first motion
			const float X=(float)x+ *vx*xRes;
			const float Y=(float)y+ *vy*yRes;

			//append the second motion vector
			*vx += GetPixel(*appFF.x, X,Y,0.f);
			*vy += GetPixel(*appFF.y, X,Y,0.f);

			++vx; ++vy;
		    }
	} else {
		//full 3D case...
		DEBUG_REPORT("3D concatenation");

		//iterate over the input flow field and change vectors...
		for (size_t z=0; z < srcFF.x->GetSizeZ(); ++z)
		  for (size_t y=0; y < srcFF.x->GetSizeY(); ++y)
		    for (size_t x=0; x < srcFF.x->GetSizeX(); ++x) {
			//it holds: *vx == srcFF.x->GetVoxel(x,y,z)

			//position after the first motion
			const float X=(float)x+ *vx*xRes;
			const float Y=(float)y+ *vy*yRes;
			const float Z=(float)z+ *vz*zRes;

			//append the second motion vector
			*vx += GetPixel(*appFF.x, X,Y,Z);
			*vy += GetPixel(*appFF.y, X,Y,Z);
			*vz += GetPixel(*appFF.z, X,Y,Z);

			++vx; ++vy; ++vz;
		    }
	}
#ifdef MITOGEN_DEBUG
	gettimeofday(&t2,NULL);
	t2.tv_sec-=t1.tv_sec;
	T1=((float)t1.tv_usec / 1000000.0f);
	T2=(float)t2.tv_sec + ((float)t2.tv_usec / 1000000.0f);
	REPORT("concatenation of flow fields took " << T2-T1 << " seconds");
#endif
}


template <class FT>
void AddFlowFields(FlowField<FT> const &srcFF,
		   FlowField<FT> const &appFF,
		   FlowField<FT> &resFF)
{
	//makes a copy of srcFF into the resFF
	//then it calls AddFlowFields(resFF,appFF);

	//but first, sanitary checks
	if (!srcFF.isConsistent())
		throw ERROR_REPORT("First motion flow field is not consistent.");
	if (!appFF.isConsistent())
		throw ERROR_REPORT("Added motion flow field is not consistent.");
	if (srcFF.x->GetSize() != appFF.x->GetSize())
		throw ERROR_REPORT("Sizes of input flow fields do not match!");
	if (srcFF.x->GetResolution().GetRes() != appFF.x->GetResolution().GetRes())
		throw ERROR_REPORT("Resolutions of input flow fields do not match!");
	if (srcFF.x->GetOffset() != appFF.x->GetOffset())
		throw ERROR_REPORT("Offsets of input flow fields do not match!");

	//creates brand new FF or just copies the content from srcFF
	//note: owing to the consistency checks, the *srcFF.x and *srcFF.y exists now
	if (!resFF.x) resFF.x=new i3d::Image3d<FT>(*srcFF.x); else *resFF.x=*srcFF.x;
	if (!resFF.y) resFF.y=new i3d::Image3d<FT>(*srcFF.y); else *resFF.y=*srcFF.y;
	if (srcFF.z) {
		//3D case
		if (!resFF.z) resFF.z=new i3d::Image3d<FT>(*srcFF.z); else *resFF.z=*srcFF.z;
	} else {
		//2D case
		if (resFF.z) {
			delete resFF.z;
			resFF.z=NULL;
		}
	}

	AddFlowFields(resFF,appFF);
}


template <class FT>
void AddFlowFields(FlowField<FT> &srcFF,
		   FlowField<FT> const &appFF)
{
	if (!srcFF.isConsistent())
		throw ERROR_REPORT("First motion flow field is not consistent.");
	if (!appFF.isConsistent())
		throw ERROR_REPORT("Added motion flow field is not consistent.");
	if (srcFF.x->GetSize() != appFF.x->GetSize())
		throw ERROR_REPORT("Sizes of input flow fields do not match!");
	if (srcFF.x->GetResolution().GetRes() != appFF.x->GetResolution().GetRes())
		throw ERROR_REPORT("Resolutions of input flow fields do not match!");
	if (srcFF.x->GetOffset() != appFF.x->GetOffset())
		throw ERROR_REPORT("Offsets of input flow fields do not match!");

	//short-cuts to the data, the x-axis
	FT *v=srcFF.x->GetFirstVoxelAddr();
	const FT *a=appFF.x->GetFirstVoxelAddr();

	//iterate over the input flow field and change vectors...
	for (size_t i=0; i < srcFF.x->GetImageSize(); ++i) {
		//it holds: *v == srcFF.x->GetVoxel(i)
		*v += *a;
		++v; ++a;
	}

	//the y-axis
	v=srcFF.y->GetFirstVoxelAddr();
	a=appFF.y->GetFirstVoxelAddr();
	for (size_t i=0; i < srcFF.x->GetImageSize(); ++i, ++v, ++a) *v += *a;

	//the z-axis possibly
	if (srcFF.z) {
		v=srcFF.z->GetFirstVoxelAddr();
		a=appFF.z->GetFirstVoxelAddr();
		for (size_t i=0; i < srcFF.x->GetImageSize(); ++i, ++v, ++a) *v += *a;
	}
}


template <class VT, class FT>
void InitFlowField(i3d::Image3d<VT> const &img, FlowField<FT> &FF)
{
	//remove the old content of the FF
	if (FF.x) delete FF.x;
	if (FF.y) delete FF.y;
	if (FF.z) delete FF.z;

	//prepare new ones, adjust their metadata and init with zeros
	FF.x = new i3d::Image3d<FT>;
	FF.x->CopyMetaData(img);
	FF.x->MakeRoom(img.GetSize());
	FF.x->GetVoxelData()=0;

	FF.y = new i3d::Image3d<FT>;
	FF.y->CopyMetaData(img);
	FF.y->MakeRoom(img.GetSize());
	FF.y->GetVoxelData()=0;

	if (img.GetSizeZ() > 1) {
		FF.z = new i3d::Image3d<FT>;
		FF.z->CopyMetaData(img);
		FF.z->MakeRoom(img.GetSize());
		FF.z->GetVoxelData()=0;
	} else FF.z=NULL;
}


template <class FT>
void SaveFlowField(const FlowField<FT>* FF,
		   const char *namePrefix,
		   const char *name,
			float maxVectorLength)
{
	//if no flow field is given, quit the function
	if (FF == NULL) return;

	//otherwise, first do some checks prior exporting
	if (!FF->isConsistent())
		throw ERROR_REPORT("Flow field is not consistent.");

	if ((namePrefix == NULL) && (name == NULL))
		throw ERROR_REPORT("No file name is provided.");
	
	//get rid of NULL pointers
	char empty_string='_';
	if (namePrefix == NULL) namePrefix=&empty_string;
	if (name == NULL) name=&empty_string;

	DEBUG_REPORT("Saving 4 files with the prefix " << namePrefix << "_" << name << "_");

	//exports flow field element-wise
	char fn[50];
	sprintf(fn, "%s_%s_X.ics", namePrefix, name);
	FF->x->SaveImage(fn);

	sprintf(fn, "%s_%s_Y.ics", namePrefix, name);
	FF->y->SaveImage(fn);

	if (FF->z) {
		sprintf(fn, "%s_%s_Z.ics", namePrefix, name);
		FF->z->SaveImage(fn);
	}

	//export colour visualization of the flow field
	i3d::Image3d<i3d::RGB> colourFlow;
	ColorRepresentationOf2DFlow(colourFlow,*FF,maxVectorLength);

	sprintf(fn, "%s_%s_colour.ics", namePrefix, name);
	colourFlow.SaveImage(fn);
}


template <class FT>
void DescribeTranslation(FlowField<FT> &FF,
			 i3d::Vector3d<float> const &shift,
			 FlowField<FT>* const eFF)
{
	//sanitary checks...
	if (!FF.isConsistent())
		throw ERROR_REPORT("Flow field is not consistent.");

	if (eFF) {
		if (!eFF->isConsistent())
			throw ERROR_REPORT("Exporting flow field is not consistent.");

		if (!eFF->isAligned(FF))
			throw ERROR_REPORT("Grid of the exporting flow field is not aligned.");
	}

	REPORT("adding vector " << shift << " to the flow field");

	//time-saver
	const size_t size=FF.x->GetImageSize();
	
	//short cut to the data
	FT *v=FF.x->GetFirstVoxelAddr();
	for (size_t i=0; i < size; ++i,++v) *v += shift.x;

	v=FF.y->GetFirstVoxelAddr();
	for (size_t i=0; i < size; ++i,++v) *v += shift.y;

	if (FF.z) {
		v=FF.z->GetFirstVoxelAddr();
		for (size_t i=0; i < size; ++i,++v) *v += shift.z;
	}

	//is exporting flow field requested?
	if (eFF) {
		IMCopyFromVOI_t hints;
		IMCopyFromVOI_GetHints(*(FF.x),*(eFF->x),hints);

		const FT zero=0.f;
		IMCopyFromVOI_Set(*(eFF->x),hints,shift.x,&zero);
		IMCopyFromVOI_Set(*(eFF->y),hints,shift.y,&zero);
		if (eFF->z) IMCopyFromVOI_Set(*(eFF->z),hints,shift.z,&zero);
	}
}


template <class FT>
void DescribeRotation(FlowField<FT> &FF,
		      i3d::Vector3d<float> const &centre,
		      const float angle,
		      FlowField<FT>* const eFF)
{
	//sanitary checks...
	if (!FF.isConsistent())
		throw ERROR_REPORT("Flow field is not consistent.");

	if (eFF) {
		if (!eFF->isConsistent())
			throw ERROR_REPORT("Exporting flow field is not consistent.");

		if (!eFF->isAligned(FF))
			throw ERROR_REPORT("Grid of the exporting flow field is not aligned.");
	}

	REPORT("adding rotation by " << angle*180.0/3.14159 << "deg clockwise around " \
		<< centre << " in microns");

	//time-savers :-)
	//note: the rotation is "flat" - only in xy
	const float xRes=FF.x->GetResolution().GetX();
	const float yRes=FF.x->GetResolution().GetY();
	const float xOff=FF.x->GetOffset().x;
	const float yOff=FF.x->GetOffset().y;

	//short-cut to the data
	FT *vx=FF.x->GetFirstVoxelAddr();
	FT *vy=FF.y->GetFirstVoxelAddr();

	//if exporting flow field:
	//
	//IMCopyFromVOI_t hints;
	//if (eFF) IMCopyFromVOI_GetHints(*(FF.x),*(eFF->x),hints);
	//
	//the same except for less computations:
	i3d::Vector3d<int> offset;
	if (eFF) {
		const i3d::Vector3d<float> res=FF.x->GetResolution().GetRes();
		offset.x=(int)roundf((FF.x->GetOffset().x - eFF->x->GetOffset().x) * res.x);
		offset.y=(int)roundf((FF.x->GetOffset().y - eFF->x->GetOffset().y) * res.y);
		offset.z=(int)roundf((FF.x->GetOffset().z - eFF->x->GetOffset().z) * res.z);

		//we should also zero the whole eFF because the translation
		//may not happen everywhere in the eFF (when eFF overlaps
		//FF only partly) and so the following for-cycles may not touch
		//some parts of eFF kepping it to what it already has
		eFF->x->GetVoxelData()=0.f;
		eFF->y->GetVoxelData()=0.f;
		if (eFF->z) eFF->z->GetVoxelData()=0.f;
	}

	//go over the flow field images and update the vectors
	for (size_t z=0; z < FF.x->GetSizeZ(); ++z)
	  for (size_t y=0; y < FF.x->GetSizeY(); ++y)
	    for (size_t x=0; x < FF.x->GetSizeX(); ++x) {
		//it holds: *vx == FF.x->GetVoxel(x,y,z)

		//now turn the pixel coordinate into a relative micron coordinate
		const float X=(float)x/xRes + xOff - centre.x;
		const float Y=(float)y/yRes + yOff - centre.y;

		//new relative position after rotation
		float nX=cos(angle)*X - sin(angle)*Y;
		float nY=sin(angle)*X + cos(angle)*Y;

		//make it a translation vector from the old place to the new one
		//and add it to the flow field vector
		*vx += nX-X;
		*vy += nY-Y;

		//is the examined x,y,z position inside the eFF?
		if ( (eFF) && (eFF->x->Include(offset + i3d::Vector3d<int>(x,y,z))) ) {
			const size_t i=i3d::GetIndex(offset.x+x,
						     offset.y+y,
						     offset.z+z,
						     eFF->x->GetSize());
			eFF->x->SetVoxel(i,nX-X);
			eFF->y->SetVoxel(i,nY-Y);
			//eFF->z->SetVoxel(i,0.f); //process it later
		}

		++vx; ++vy;
	    }

	//finalize the z-element flow field (rotation is XY-flat)
	if ((eFF) && (eFF->z)) eFF->z->GetVoxelData()=0.f;
}


/**
 * uncomment this to turn on the post-smoothing of created flow field
 * in the DescribeRadialFlow() function
 */
//#define GTGEN_WITH_SMOOTH_RADIAL_FLOW_FIELD

#ifdef GTGEN_WITH_SMOOTH_RADIAL_FLOW_FIELD
/**
 * a helper static structures so that one does not need
 * to allocate and initiate them over and over again
 *
 * it is used in the function DescribeRadialFlow()
 */
static bool pnInitiated=false;

/**
 * a common to all cells structure for non-rigid deformations;
 * represents Gaussian kernel for post-processing of created flow field
 *
 * it is used in the function DescribeRadialFlow()
 */
static i3d::Separable3dFilter<float> pnKernel;

/**
 * a common to all cells structure for non-rigid deformations;
 * represents boundary values for post-processing of created flow field
 *
 * it is used in the function DescribeRadialFlow()
 */
static i3d::BorderPadding<float> pnBoundaries;
#endif

template <class FT>
void DescribeRadialFlow(FlowField<FT> &FF,
			std::vector< i3d::Vector3d<float> /**/> const &BPList,
			i3d::Vector3d<float> const &BPCentre,
			const size_t BPOuterCnt,
			const float *hintArray,
			const int hintInc,
			const bool compensation,
			FlowField<FT>* const eFF)
{
	//consistency checks
	if (!FF.isConsistent())
		throw ERROR_REPORT("Flow field is not consistent.");
	if (!FF.z)
		throw ERROR_REPORT("The input flow field is not 3D.");

	if (eFF) {
		if (!eFF->isConsistent())
			throw ERROR_REPORT("Exporting flow field is not consistent.");

		if (!eFF->isAligned(FF))
			throw ERROR_REPORT("Grid of the exporting flow field is not aligned.");
	}

	if (BPOuterCnt > BPList.size())
		throw ERROR_REPORT("There are more outer boundary points than is the length of the list.");
	if (hintArray == NULL)
		throw ERROR_REPORT("The hintArray points to NULL.");

	if (hintInc < 0)
	{
		REPORT("Warning: The hintInc is negative, is it Okay?");
	}

	//draw the flow field first here, the values here may be accumulated
	FlowField<FT> myFF;

	//this makes it zeroed and of appropriate size
	InitFlowField(*FF.x,myFF);

	//how many vectors are actually stored in every pixel of the myFF
	i3d::Image3d<float> *tmpFFz=new i3d::Image3d<float>();
	i3d::Image3d<float> &myFFcounts=*tmpFFz;
	myFFcounts.CopyMetaData(*FF.x);
	myFFcounts.Clear();
	//note: myFFcounts looks like a local variable but it is actually
	//allocated on the heap, hence its content persists after this function ends

	REPORT("adding grow/shrink");
	/*
	DEBUG_REPORT("going to add to a flow field at " << FF.x->GetOffset() << " of size " \
	       << i3d::PixelsToMicrons(FF.x->GetSize(),FF.x->GetResolution())  << " in microns");
	*/

	//time-savers ;-)
	const float xRes=FF.x->GetResolution().GetX();
	const float yRes=FF.x->GetResolution().GetY();
	const float zRes=FF.x->GetResolution().GetZ();
	const float xOff=FF.x->GetOffset().x;
	const float yOff=FF.x->GetOffset().y;
	const float zOff=FF.x->GetOffset().z;

	//pixel position of the BPCentre
	const int Xc=(int)roundf( (BPCentre.x-xOff)*xRes );
	const int Yc=(int)roundf( (BPCentre.y-yOff)*yRes );
	const int Zc=(int)roundf( (BPCentre.z-zOff)*zRes );

	//sweeping pointer within the hintArray
	const float *myHintPtr=hintArray;

	//required when 6-neighborhood is used for dilations of the radial lines
	//static const int dilationHints[7][3]={{0,0,0},{-1,0,0},{+1,0,0},{0,-1,0},{0,+1,0},{0,0,-1},{0,0,+1}};

#ifdef MITOGEN_DEBUG
	//stats from values used for this flow field
	/*
	double meanHintVal=0.;
	double meanCompensatedHintVal=0.;
	double sum=0.; //for computing variance
	double sum2=0.;
	double max=-1000.; //for extremas
	double min=1000.;
	//int histo[1001]; //histogram //TEST
	//for (size_t i=0; i < 1001; ++i) histo[i]=0; //TEST
	*/

	//indicator to show (the "out of image") debug message just once
	bool NoMoreDbgReports=false;

	//time-measurement
	struct timeval t1,t2,t3;
	float T1,T2;
	gettimeofday(&t1,NULL);
#endif
	//now, go over all requested (outer) boundary points
	for (size_t i=0; i < BPOuterCnt; ++i, myHintPtr+=hintInc) {
		//project given point to the flow field
		const int X=(int)roundf( (BPList[i].x-xOff)*xRes );
		const int Y=(int)roundf( (BPList[i].y-yOff)*yRes );
		const int Z=(int)roundf( (BPList[i].z-zOff)*zRes );

		//vector towards the centre, in pixels
		float dx=Xc - X;
		float dy=Yc - Y;
		float dz=Zc - Z;
		//and its size
		const float sz=sqrtf(dx*dx + dy*dy + dz*dz);

		//vector towards the centre, in microns
		//
		//computed from "ideal" positions which are typically slightly
		//offset from the pixel grid and they, if placed on it, will
		//point not exactly to the centre
		//float vdx=BPCentre.x - BPList[i].x;
		//float vdy=BPCentre.y - BPList[i].y;
		//float vdz=BPCentre.z - BPList[i].z;
		float vdx=-dx/xRes; //re-computed from "valid pixel" vector
		float vdy=-dy/yRes;
		float vdz=-dz/zRes;
		const float vsz=sqrtf(vdx*vdx + vdy*vdy + vdz*vdz);

		//normalize both vectors
		dx/=sz; dy/=sz; dz/=sz;
		vdx/=vsz; vdy/=vsz; vdz/=vsz;

		//situation:
		//the routine now places vectors into the flow field
		//along following direction given by (dx,dy,dz); the placed
		//vectors are pointing in the direction given by (vdx,vdy,vdz)

		//adjust the size of (vdx,vdy,vdz) vector to compensate for
		//different sector volumes represented with this vector if it
		//points outward and inward the cell
		//
		//the vector at cell boundary should be of length *myHintPtr,
		//so that the boundary point, which is already vsz microns away
		//from the centre, should end up vsz+*myHintPtr microns away
		//from the it; but it will add more volume than what was expected
		//to be added, hence we adjust the length of the added vector so
		//that it adds the expected volume... the new length is:
		const float r3=cbrt( vsz*vsz * (*myHintPtr *3.f + vsz) );
		const float newHintVal=(compensation)? r3-vsz : *myHintPtr;
		/*
		if (compensation)
			DEBUG_REPORT(*myHintPtr << "um at radius " << vsz
				<< " is compensated to " << newHintVal << "um");
		*/

		//enforces that given radial line is shifted by 1px in all directions,
		//the radial line is dilated in this way with:
		// full 26 neighborhood
		for (int zz=-1; zz <= +1; ++zz)
		  for (int yy=-1; yy <= +1; ++yy)
		    for (int xx=-1; xx <= +1; ++xx)
		{
		/*
		// smaller 6 neighborhood
		for (int qq=0; qq < 7; ++qq)
		{
		int xx=dilationHints[qq][0];
		int yy=dilationHints[qq][1];
		int zz=dilationHints[qq][2];
		{
		*/
			//last visited position, to avoid placing the same vector at the same place twice
			int lastX=999999.f;
			int lastY=999999.f;
			int lastZ=999999.f;

			//now place it along the line, start well before the cell boundary
			//k goes in pixels, not in microns
			//for (float k=-4.f; k < (sz-1.f); k+=0.5f) {
			//can be shorted since we are not smoothing in the end
			for (float k=-3.f; k < (sz-1.f); k+=0.5f) {
				//current position in pixels
				const int x=roundf((float)X + k*dx) +xx;
				const int y=roundf((float)Y + k*dy) +yy;
				const int z=roundf((float)Z + k*dz) +zz;
				//note: k is in pixels

				if ((x != lastX) || (y != lastY) || (z != lastZ)) {
					//new position:
					//is it inside the image?
					if (!myFFcounts.Include(x,y,z)) {
#ifdef MITOGEN_DEBUG
					  if (!NoMoreDbgReports)
					  {
						REPORT("Warning: Should prepare vectors outside of the flow field.");
					  }
					  NoMoreDbgReports=true;
#endif
					  //delete tmpFFz;
					  //throw ERROR_REPORT("Can't prepare vectors outside of the flow field.");
					} else {
						//weight
						//the commented out code is better, but this one
						//is faster and just works well...
						const float w=newHintVal * (0.5f*cos(k/sz*3.14159f)+0.5f); //ORIG
						/*
						const float w=(k < 0.f)?
							newHintVal * (-0.5f*cos(k/sz*3.14159f)+1.5f):
							newHintVal * (0.5f*cos(k/sz*3.14159f)+0.5f);
						*/

						//linearized position
						const size_t index=myFFcounts.GetIndex(x,y,z);

						//add it to the accumulating flow field
						myFF.x->SetVoxel(index, w*vdx + myFF.x->GetVoxel(index));
						myFF.y->SetVoxel(index, w*vdy + myFF.y->GetVoxel(index));
						myFF.z->SetVoxel(index, w*vdz + myFF.z->GetVoxel(index));
						myFFcounts.SetVoxel(index, 1.f+myFFcounts.GetVoxel(index));
					}

					//remember the last accessed position
					lastX=x; lastY=y; lastZ=z;
				}
			} //for the radial line rendering
		} //for shifts/dilation of the radial line
#ifdef MITOGEN_DEBUG
		/*
		meanHintVal+=*myHintPtr;
		meanCompensatedHintVal+=newHintVal;
		sum+=*myHintPtr;
		sum2+=*myHintPtr * *myHintPtr;
		if (*myHintPtr > max) max=*myHintPtr;
		if (*myHintPtr < min) min=*myHintPtr;
		//++histo[ (int)floorf(*myHintPtr*100.f)+500 ]; //TEST
		*/
#endif
	}

	//TODO VLADO REMOVE AFTER DEBUG
	//myFFcounts.SaveImage("myFFcounts.ics");

#ifdef MITOGEN_DEBUG
	/*
	double var=( sum2 - sum*sum/(double)BPOuterCnt ) / (double)BPOuterCnt;
	REPORT("mean from used hint values is " << meanHintVal/(double)BPOuterCnt
		<< " +- " << sqrt(var));
	REPORT("min, max from used hint values are " << min << ", " << max);
	REPORT("mean from compensated hint values is " << meanCompensatedHintVal/(double)BPOuterCnt);
	//for (signed int i=0; i < 1001; ++i) std::cout << (float)(i-500)/100.f << " " << histo[i] << "\n"; //TEST
	*/

	gettimeofday(&t2,NULL);
	t2.tv_sec-=t1.tv_sec;
	T1=((float)t1.tv_usec / 1000000.0f);
	T2=(float)t2.tv_sec + ((float)t2.tv_usec / 1000000.0f);
	REPORT("initial filling took " << T2-T1 << " seconds");
#endif

	//the myFF holds now accumulated vectors, make them average
	//vectors by dividing with corresponding pixel values from myFFcounts
	float *Vx=myFF.x->GetFirstVoxelAddr();
	float *Vy=myFF.y->GetFirstVoxelAddr();
	float *Vz=myFF.z->GetFirstVoxelAddr();
	const float *Cnt=myFFcounts.GetFirstVoxelAddr();

	//the divide (= averaging) routine
	for (size_t i=0; i < myFFcounts.GetImageSize(); ++i) {
		if (*Cnt > 0) {
			*Vx /= *Cnt;
			*Vy /= *Cnt;
			*Vz /= *Cnt;
		}

		++Vx; ++Vy; ++Vz;
		++Cnt;
	}

#ifdef MITOGEN_DEBUG
	gettimeofday(&t3,NULL);
	t3.tv_sec-=t1.tv_sec;
	T1=(float)t3.tv_sec + ((float)t3.tv_usec / 1000000.0f);
	REPORT("averaging took " << T1-T2 << " seconds");
	T2=T1;
#endif

#ifdef GTGEN_WITH_SMOOTH_RADIAL_FLOW_FIELD
	if (!pnInitiated) {
		//first use of this function, setup helper structures

		//prepare the helper internal structures above this function
		EstimateGaussKernel(pnKernel,
			1.0f,1.0f, //x,y and z on the next line
			1.0f * FF.x->GetResolution().GetZ() / FF.x->GetResolution().GetX()
			);
		PrepareZeroBoundaryForFilter(pnKernel,pnBoundaries);
		pnInitiated=true;
	}

	//finalize the flow field with little smoothing,
	//note: we rotate buffers... and make use of the heap one tmpFFz too
	SeparableConvolution(*myFF.z,*tmpFFz,pnKernel,pnBoundaries);
	SeparableConvolution(*myFF.y,*myFF.z,pnKernel,pnBoundaries);
	SeparableConvolution(*myFF.x,*myFF.y,pnKernel,pnBoundaries);

	delete myFF.x;
	myFF.x=myFF.y;
	myFF.y=myFF.z;
	myFF.z=tmpFFz;

#ifdef MITOGEN_DEBUG
	gettimeofday(&t2,NULL);
	t2.tv_sec-=t1.tv_sec;
	T2=(float)t2.tv_sec + ((float)t2.tv_usec / 1000000.0f);
	REPORT("post-smoothing with (1,1," << FF.x->GetResolution().GetZ()/FF.x->GetResolution().GetX()
		<< ") took " << T2-T1 << " seconds");
#endif
#else //for the #ifdef GTGEN_WITH_SMOOTH_RADIAL_FLOW_FIELD
	//clean up the temporal image
	delete tmpFFz;
#endif

	//now add the myFF to the FF flow field
	/*
	Vx=FF.x->GetFirstVoxelAddr();
	Vy=FF.y->GetFirstVoxelAddr();
	Vz=FF.z->GetFirstVoxelAddr();

	const float *cVx=myFF.x->GetFirstVoxelAddr();
	const float *cVy=myFF.y->GetFirstVoxelAddr();
	const float *cVz=myFF.z->GetFirstVoxelAddr();

	//the addition itself
	for (size_t i=0; i < FF.x->GetImageSize(); ++i) {
		*Vx += *cVx;
		*Vy += *cVy;
		*Vz += *cVz;

		++Vx; ++Vy; ++Vz;
		++cVx; ++cVy; ++cVz;
	}
	//the next code is actually 2x faster
	*/
	//the addition itself
	Vx=FF.x->GetFirstVoxelAddr();
	const float *cVx=myFF.x->GetFirstVoxelAddr();
	for (size_t i=0; i < FF.x->GetImageSize(); ++i) {
		*Vx += *cVx;
		++Vx; ++cVx;
	}
	Vx=FF.y->GetFirstVoxelAddr();
	cVx=myFF.y->GetFirstVoxelAddr();
	for (size_t i=0; i < FF.y->GetImageSize(); ++i) {
		*Vx += *cVx;
		++Vx; ++cVx;
	}
	Vx=FF.z->GetFirstVoxelAddr();
	cVx=myFF.z->GetFirstVoxelAddr();
	for (size_t i=0; i < FF.z->GetImageSize(); ++i) {
		*Vx += *cVx;
		++Vx; ++cVx;
	}

#ifdef MITOGEN_DEBUG
	gettimeofday(&t3,NULL);
	t3.tv_sec-=t1.tv_sec;
	T1=(float)t3.tv_sec + ((float)t3.tv_usec / 1000000.0f);
	REPORT("adding flow fields took " << T1-T2 << " seconds");
#endif

	//is exporting flow field requested?
	if (eFF) {
		IMCopyFromVOI_t hints;
		IMCopyFromVOI_GetHints(*(FF.x),*(eFF->x),hints);

		const FT zero=0.f;
		IMCopyFromVOI_Apply(*(myFF.x),*(eFF->x),hints,&zero);
		IMCopyFromVOI_Apply(*(myFF.y),*(eFF->y),hints,&zero);
		if (eFF->z) IMCopyFromVOI_Apply(*(myFF.z),*(eFF->z),hints,&zero);
		//note: myFF.z always exists here in this function
	}

	/*
	//TODO VLADO REMOVE AFTER DEBUG
	i3d::Image3d<i3d::RGB> showFlow;
	ColorRepresentationOf2DFlow(showFlow,FF,cellLookDistance);
	char fn[50]="myFF.ics";
	showFlow.SaveImage(fn);
	*/
}


/* this method has been implemented by Andres Bruhn, University of Saarland */
template <class itype, class fptype> void VectorToRGB(fptype x,       /* in  : x-component */
                                                        fptype y,       /* in  : y-component */
                                                        itype * R,      /* out : red component */
                                                        itype * G,      /* out : green component */
                                                        itype * B       /* out : blue component */
    )
{
    fptype Pi;                  /* pit */
    fptype amp;                 /* amplitude */
    fptype phi;                 /* phase */
    fptype alpha, beta;         /* weights for linear interpolation */

    /* set pi */
    Pi = 2.0 * acos(0.0);

    //x=-x;

    /* determine amplitude and phase (cut amp at 1) */
    amp = sqrt(x * x + y * y);
    if (amp > 1)
        amp = 1;
    if (x == 0.0)
        if (y >= 0.0)
            phi = 0.5 * Pi;
        else
            phi = 1.5 * Pi;
    else if (x > 0.0)
        if (y >= 0.0)
            phi = atan(y / x);
        else
            phi = 2.0 * Pi + atan(y / x);
    else
        phi = Pi + atan(y / x);

    phi = phi / 2.0;

    // interpolation between red (0) and blue (0.25 * Pi)
    if ((phi >= 0.0) && (phi < 0.125 * Pi))
    {
        beta = phi / (0.125 * Pi);
        alpha = 1.0 - beta;
        *R = (itype) floor(amp * (alpha * 255.0 + beta * 255.0));
        *G = (itype) floor(amp * (alpha * 0.0 + beta * 0.0));
        *B = (itype) floor(amp * (alpha * 0.0 + beta * 255.0));
    }
    if ((phi >= 0.125 * Pi) && (phi < 0.25 * Pi))
    {
        beta = (phi - 0.125 * Pi) / (0.125 * Pi);
        alpha = 1.0 - beta;
        *R = (itype) floor(amp * (alpha * 255.0 + beta * 64.0));
        *G = (itype) floor(amp * (alpha * 0.0 + beta * 64.0));
        *B = (itype) floor(amp * (alpha * 255.0 + beta * 255.0));
    }
    // interpolation between blue (0.25 * Pi) and green (0.5 * Pi)
    if ((phi >= 0.25 * Pi) && (phi < 0.375 * Pi))
    {
        beta = (phi - 0.25 * Pi) / (0.125 * Pi);
        alpha = 1.0 - beta;
        *R = (itype) floor(amp * (alpha * 64.0 + beta * 0.0));
        *G = (itype) floor(amp * (alpha * 64.0 + beta * 255.0));
        *B = (itype) floor(amp * (alpha * 255.0 + beta * 255.0));
    }
    if ((phi >= 0.375 * Pi) && (phi < 0.5 * Pi))
    {
        beta = (phi - 0.375 * Pi) / (0.125 * Pi);
        alpha = 1.0 - beta;
        *R = (itype) floor(amp * (alpha * 0.0 + beta * 0.0));
        *G = (itype) floor(amp * (alpha * 255.0 + beta * 255.0));
        *B = (itype) floor(amp * (alpha * 255.0 + beta * 0.0));
    }
    // interpolation between green (0.5 * Pi) and yellow (0.75 * Pi)
    if ((phi >= 0.5 * Pi) && (phi < 0.75 * Pi))
    {
        beta = (phi - 0.5 * Pi) / (0.25 * Pi);
        alpha = 1.0 - beta;
        *R = (itype) floor(amp * (alpha * 0.0 + beta * 255.0));
        *G = (itype) floor(amp * (alpha * 255.0 + beta * 255.0));
        *B = (itype) floor(amp * (alpha * 0.0 + beta * 0.0));
    }
    // interpolation between yellow (0.75 * Pi) and red (Pi)
    if ((phi >= 0.75 * Pi) && (phi <= Pi))
    {
        beta = (phi - 0.75 * Pi) / (0.25 * Pi);
        alpha = 1.0 - beta;
        *R = (itype) floor(amp * (alpha * 255.0 + beta * 255.0));
        *G = (itype) floor(amp * (alpha * 255.0 + beta * 0.0));
        *B = (itype) floor(amp * (alpha * 0.0 + beta * 0.0));
    }

    /* check RGB range */
    if (*R < 0)
        *R = 0;
    if (*G < 0)
        *G = 0;
    if (*B < 0)
        *B = 0;
    if (*R > 255)
        *R = 255;
    if (*G > 255)
        *G = 255;
    if (*B > 255)
        *B = 255;
}

void ColorRepresentationOf2DFlow(i3d::Image3d<i3d::RGB> &Output, FlowField<float> const &FF,
				const float normalization)
{
    float *x, *y, *last;
    i3d::RGB *rgb;
    int r(0), g(0), b(0);

    Output.CopyMetaData(*FF.x);
    //Output.MakeRoom(FF.x->GetSize()); -- already done in CopyMetaData()

    x = FF.x->GetFirstVoxelAddr();
    last = FF.x->GetVoxelAddr(FF.x->GetImageSize());
    y = FF.y->GetFirstVoxelAddr();
    rgb = Output.GetFirstVoxelAddr();

    while (x != last)
    {
        VectorToRGB(*x / normalization, *y / normalization, &r, &g, &b);
        rgb->red = static_cast <i3d::GRAY8> (r);
        rgb->green = static_cast <i3d::GRAY8> (g);
        rgb->blue = static_cast <i3d::GRAY8> (b);
        ++x;
        ++y;
        ++rgb;
    }
}


template <class VT>
void PutPixel(i3d::Image3d<VT> &img,const float x,const float y,const float z,const VT value)
{
	//nearest not-greater integer coordinate, "o" in the picture in docs
	//X,Y,Z will be coordinate of the voxel no. 2
	const int X=static_cast<int>(floorf(x));
	const int Y=static_cast<int>(floorf(y));
	const int Z=static_cast<int>(floorf(z));
	//now we can write only to pixels at [X or X+1,Y or Y+1,Z or Z+1]

	//quit if too far from the "left" borders of the image
	//as we wouldn't be able to draw into the image anyway
	if ((X < -1) || (Y < -1) || (Z < -1)) return;

	//residual fraction of the input coordinate
	const float Xfrac=x - static_cast<float>(X);
	const float Yfrac=y - static_cast<float>(Y);
	const float Zfrac=z - static_cast<float>(Z);

	//the weights
	float A=0.0f,B=0.0f,C=0.0f,D=0.0f; //for 2D

	//x axis:
	A=D=Xfrac;
	B=C=1.0f - Xfrac;

	//y axis:
	A*=1.0f - Yfrac;
	B*=1.0f - Yfrac;
	C*=Yfrac;
	D*=Yfrac;

	//z axis:
	float A_=A,B_=B,C_=C,D_=D;
	A*=1.0f - Zfrac;
	B*=1.0f - Zfrac;
	C*=1.0f - Zfrac;
	D*=1.0f - Zfrac;
	A_*=Zfrac;
	B_*=Zfrac;
	C_*=Zfrac;
	D_*=Zfrac;

	//portions of the value in a bit more organized form, w[z][y][x]
	const float w[2][2][2]={{{B*(float)value, A*(float)value}, {C*(float)value, D*(float)value}},
				{{B_*(float)value,A_*(float)value},{C_*(float)value,D_*(float)value}}};

	//inserting into the output image,
	//for (int zi=0; zi < 2; ++zi) if (Z+zi < (signed)img.GetSizeZ()) { //shortcut for 2D cases to avoid some computations...
	for (int zi=0; zi < 2; ++zi)
	  for (int yi=0; yi < 2; ++yi)
	    for (int xi=0; xi < 2; ++xi)
		if (img.Include(X+xi,Y+yi,Z+zi)) {
			//if we got here then we can safely change coordinate types
			const float v=(float)img.GetVoxel((size_t)X+xi,(size_t)Y+yi,(size_t)Z+zi) + w[zi][yi][xi];
			img.SetVoxel((size_t)X+xi,(size_t)Y+yi,(size_t)Z+zi,static_cast<VT>(v));
		}
	//}
}


template <class VT>
VT GetPixel(i3d::Image3d<VT> const &img,const float x,const float y,const float z)
{
	/*
	//nearest neighbor:
	int X=static_cast<int>(roundf(x));
	int Y=static_cast<int>(roundf(y));
	int Z=static_cast<int>(roundf(z));

	if (img.Include(X,Y,Z)) return(img.GetVoxel(X,Y,Z));
	else return(0);
	*/

	//nearest not-greater integer coordinate, "o" in the picture in docs
	//X,Y,Z will be coordinate of the voxel no. 2
	const int X=static_cast<int>(floorf(x));
	const int Y=static_cast<int>(floorf(y));
	const int Z=static_cast<int>(floorf(z));
	//now we can write only to pixels at [X or X+1,Y or Y+1,Z or Z+1]

	//quit if too far from the "left" borders of the image
	//as we wouldn't be able to draw into the image anyway
	if ((X < -1) || (Y < -1) || (Z < -1)) return (0);

	//residual fraction of the input coordinate
	const float Xfrac=x - static_cast<float>(X);
	const float Yfrac=y - static_cast<float>(Y);
	const float Zfrac=z - static_cast<float>(Z);

	//the weights
	float A=0.0f,B=0.0f,C=0.0f,D=0.0f; //for 2D

	//x axis:
	A=D=Xfrac;
	B=C=1.0f - Xfrac;

	//y axis:
	A*=1.0f - Yfrac;
	B*=1.0f - Yfrac;
	C*=Yfrac;
	D*=Yfrac;

	//z axis:
	float A_=A,B_=B,C_=C,D_=D;
	A*=1.0f - Zfrac;
	B*=1.0f - Zfrac;
	C*=1.0f - Zfrac;
	D*=1.0f - Zfrac;
	A_*=Zfrac;
	B_*=Zfrac;
	C_*=Zfrac;
	D_*=Zfrac;

	//portions of the value in a bit more organized form, w[z][y][x]
	const float w[2][2][2]={{{B ,A },{C ,D }},
				{{B_,A_},{C_,D_}}};

	//the return value
	float v=0;

	//reading from the input image,
	//for (int zi=0; zi < 2; ++zi) if (Z+zi < (signed)img.GetSizeZ()) { //shortcut for 2D cases to avoid some computations...
	for (int zi=0; zi < 2; ++zi)
	  for (int yi=0; yi < 2; ++yi)
	    for (int xi=0; xi < 2; ++xi)
		if (img.Include(X+xi,Y+yi,Z+zi)) {
			//if we got here then we can safely change coordinate types
			v+=(float)img.GetVoxel((size_t)X+xi,(size_t)Y+yi,(size_t)Z+zi) * w[zi][yi][xi];
		}
	//}

	return ( static_cast<VT>(v) );
}


template <class VOXEL>
void IMCopyFromVOI_GetHints(i3d::Image3d<VOXEL> const &gImg,
			    i3d::Image3d<VOXEL> const &fImg,
			    IMCopyFromVOI_t &hints)
{
	//we assume gImg and fImg are aligned: same resolution, grids overlay

	const i3d::Vector3d<float> res=gImg.GetResolution().GetRes();

	//expresses in pixels a relative position of gImg within fImg
	//if, e.g., offDiff.x is positive, then fImg is outside and to the left
	//(in x-axis) of the gImg by that amount of pixels
	i3d::Vector3d<int> offDiff;
	offDiff.x=(int)roundf((gImg.GetOffset().x - fImg.GetOffset().x) * res.x);
	offDiff.y=(int)roundf((gImg.GetOffset().y - fImg.GetOffset().y) * res.y);
	offDiff.z=(int)roundf((gImg.GetOffset().z - fImg.GetOffset().z) * res.z);

	//offset to the gImg
	hints.offset=offDiff;

	//type-casted helper vector:
	i3d::Vector3d<int> fImgGetSize(fImg.GetSize());

	//how much to skip at the beginning
	//note: i3d::max goes element-wise
	hints.skipA=i3d::min( i3d::max(offDiff,i3d::Vector3d<int>(0,0,0)),
				fImgGetSize );

	//right boundary...
	hints.work=i3d::max( i3d::min(fImgGetSize, offDiff+gImg.GetSize()),
				i3d::Vector3d<int>(0,0,0) );

	//how much to skip at the end
	hints.skipB=i3d::max(fImgGetSize-hints.work, i3d::Vector3d<int>(0,0,0));

	//how much left to work...
	hints.work -= hints.skipA;

	DEBUG_REPORT("computed this IMCopyFromVOI_t:  offset=" << hints.offset
		<< ", skipA=" << hints.skipA << ", work=" << hints.work
		<< ", skipB=" << hints.skipB);
}


///helper memset() alternative for float (was not able to create "inlined template")
inline
void memSet(float *dst, const float val, const size_t count)
{
	for (size_t i=0; i < count; ++i, ++dst) *dst=val;
}

///helper memset() alternative for i3d::GRAY8 (was not able to create "inlined template")
inline
void memSet(i3d::GRAY8 *dst, const i3d::GRAY8 val, const size_t count)
{
	for (size_t i=0; i < count; ++i, ++dst) *dst=val;
}

///helper memset() alternative for i3d::GRAY16 (was not able to create "inlined template")
inline
void memSet(i3d::GRAY16 *dst, const i3d::GRAY16 val, const size_t count)
{
	for (size_t i=0; i < count; ++i, ++dst) *dst=val;
}


template <class VOXEL>
void IMCopyFromVOI_Apply(i3d::Image3d<VOXEL> const &gImg,
			 i3d::Image3d<VOXEL> &fImg,
			 IMCopyFromVOI_t const &hints,
			 const VOXEL *skipValue)
{
	//time-savers... ;-)
	const size_t xLine=fImg.GetSizeX();
	const size_t zSlice=xLine * fImg.GetSizeY();

	//start filling the whole fImg
	VOXEL* fP=fImg.GetFirstVoxelAddr();

	//start reading from gImg at the first "work" section
	const VOXEL* gP=gImg.GetFirstVoxelAddr()
		+ i3d::GetIndex(hints.skipA-hints.offset,gImg.GetSize());

	//possibly, "skip" some openning zSlices
	if (skipValue) memSet(fP,*skipValue,hints.skipA.z*zSlice);
	fP+=hints.skipA.z*zSlice;

	//process some "inner" zSlices
	for (int z=0; z < hints.work.z; ++z) {
		//now, process xLine by xLine
		//possibly, "skip" some openning xLines
		if (skipValue) memSet(fP,*skipValue,hints.skipA.y*xLine);
		fP+=hints.skipA.y*xLine;

		//process some "inner" xLines
		for (int y=0; y < hints.work.y; ++y) {
			//process one xLine
			if (skipValue) memSet(fP,*skipValue,hints.skipA.x);
			fP+=hints.skipA.x;

			memcpy(fP,gP+y*gImg.GetSizeX(),hints.work.x*sizeof(VOXEL));
			fP+=hints.work.x;

			if (skipValue) memSet(fP,*skipValue,hints.skipB.x);
			fP+=hints.skipB.x;
		}

		//possibly, "skip" some closing xLines
		if (skipValue) memSet(fP,*skipValue,hints.skipB.y*xLine);
		fP+=hints.skipB.y*xLine;

		//adjust also the source pointer
		gP+=gImg.GetSliceSize();
	}

	//possibly, "skip" some closing zSlices
	if (skipValue) memSet(fP,*skipValue,hints.skipB.z*zSlice);
	fP+=hints.skipB.z*zSlice;
}


template <class VOXEL>
void IMCopyFromVOI_Set(i3d::Image3d<VOXEL> &fImg,
		       IMCopyFromVOI_t const &hints,
		       const VOXEL setValue,
		       const VOXEL *skipValue)
{
	//time-savers... ;-)
	const size_t xLine=fImg.GetSizeX();
	const size_t zSlice=xLine * fImg.GetSizeY();

	//start filling the whole fImg
	VOXEL* fP=fImg.GetFirstVoxelAddr();

	//possibly, "skip" some openning zSlices
	if (skipValue) memSet(fP,*skipValue,hints.skipA.z*zSlice);
	fP+=hints.skipA.z*zSlice;

	//process some "inner" zSlices
	for (int z=0; z < hints.work.z; ++z) {
		//now, process xLine by xLine
		//possibly, "skip" some openning xLines
		if (skipValue) memSet(fP,*skipValue,hints.skipA.y*xLine);
		fP+=hints.skipA.y*xLine;

		//process some "inner" xLines
		for (int y=0; y < hints.work.y; ++y) {
			//process one xLine
			if (skipValue) memSet(fP,*skipValue,hints.skipA.x);
			fP+=hints.skipA.x;

			memSet(fP,setValue,hints.work.x);
			fP+=hints.work.x;

			if (skipValue) memSet(fP,*skipValue,hints.skipB.x);
			fP+=hints.skipB.x;
		}

		//possibly, "skip" some closing xLines
		if (skipValue) memSet(fP,*skipValue,hints.skipB.y*xLine);
		fP+=hints.skipB.y*xLine;
	}

	//possibly, "skip" some closing zSlices
	if (skipValue) memSet(fP,*skipValue,hints.skipB.z*zSlice);
	fP+=hints.skipB.z*zSlice;
}



//
// explicit instantiations
//
template void ImageForwardTransformation(i3d::Image3d<bool> const &srcImg,
				i3d::Image3d<bool> &dstImg, FlowField<float> const &FF);
template void ImageForwardTransformation(i3d::Image3d<i3d::GRAY8> const &srcImg,
				i3d::Image3d<i3d::GRAY8> &dstImg, FlowField<float> const &FF);
template void ImageForwardTransformation(i3d::Image3d<i3d::GRAY16> const &srcImg,
				i3d::Image3d<i3d::GRAY16> &dstImg, FlowField<float> const &FF);
template void ImageForwardTransformation(i3d::Image3d<float> const &srcImg,
				i3d::Image3d<float> &dstImg, FlowField<float> const &FF);

template void ConcatenateFlowFields(FlowField<float> &srcFF, FlowField<float> const &appFF);
template void ConcatenateFlowFields(FlowField<float> const &srcFF, FlowField<float> const &appFF,
				    FlowField<float> &resFF);
template void AddFlowFields(FlowField<float> &srcFF, FlowField<float> const &appFF);
template void AddFlowFields(FlowField<float> const &srcFF, FlowField<float> const &appFF,
			    FlowField<float> &resFF);

template void InitFlowField(i3d::Image3d<bool> const &img, FlowField<float> &FF);
template void InitFlowField(i3d::Image3d<i3d::GRAY8> const &img, FlowField<float> &FF);
template void InitFlowField(i3d::Image3d<i3d::GRAY16> const &img, FlowField<float> &FF);
template void InitFlowField(i3d::Image3d<float> const &img, FlowField<float> &FF);

template void SaveFlowField(const FlowField<float>* FF, const char *namePrefix, const char *name, float);

template void DescribeTranslation(FlowField<float> &FF, i3d::Vector3d<float> const &shift,
				FlowField<float>* const eFF);
template void DescribeRotation(FlowField<float> &FF, i3d::Vector3d<float> const &centre, const float angle,
				FlowField<float>* const eFF);
template void DescribeRadialFlow(FlowField<float> &FF,
			std::vector< i3d::Vector3d<float> /**/> const &BPList, i3d::Vector3d<float> const &BPCentre,
			const size_t BPOuterCnt, const float *hintArray, const int hintInc, const bool compensation,
			FlowField<float>* const eFF);

template i3d::GRAY8  GetPixel(i3d::Image3d<i3d::GRAY8>  const &img,const float x,const float y,const float z);
template i3d::GRAY16 GetPixel(i3d::Image3d<i3d::GRAY16> const &img,const float x,const float y,const float z);
template float GetPixel(i3d::Image3d<float> const &img,const float x,const float y,const float z);
//PutPixel is instantianed through the ImageForwardTransformation() function

template void IMCopyFromVOI_GetHints(i3d::Image3d<float> const &gImg,i3d::Image3d<float> const &fImg,
			IMCopyFromVOI_t &hints);
template void IMCopyFromVOI_GetHints(i3d::Image3d<i3d::GRAY8> const &gImg,i3d::Image3d<i3d::GRAY8> const &fImg,
			IMCopyFromVOI_t &hints);
template void IMCopyFromVOI_GetHints(i3d::Image3d<i3d::GRAY16> const &gImg,i3d::Image3d<i3d::GRAY16> const &fImg,
			IMCopyFromVOI_t &hints);
template void IMCopyFromVOI_Apply(i3d::Image3d<float> const &gImg,i3d::Image3d<float> &fImg,
			IMCopyFromVOI_t const &hints,const float *skipValue);
template void IMCopyFromVOI_Apply(i3d::Image3d<i3d::GRAY8> const &gImg,i3d::Image3d<i3d::GRAY8> &fImg,
			IMCopyFromVOI_t const &hints,const i3d::GRAY8 *skipValue);
template void IMCopyFromVOI_Apply(i3d::Image3d<i3d::GRAY16> const &gImg,i3d::Image3d<i3d::GRAY16> &fImg,
			IMCopyFromVOI_t const &hints,const i3d::GRAY16 *skipValue);
template void IMCopyFromVOI_Set(i3d::Image3d<float> &fImg,IMCopyFromVOI_t const &hints,
			const float setValue,const float *skipValue);
template void IMCopyFromVOI_Set(i3d::Image3d<i3d::GRAY8> &fImg,IMCopyFromVOI_t const &hints,
			const i3d::GRAY8 setValue,const i3d::GRAY8 *skipValue);
template void IMCopyFromVOI_Set(i3d::Image3d<i3d::GRAY16> &fImg,IMCopyFromVOI_t const &hints,
			const i3d::GRAY16 setValue,const i3d::GRAY16 *skipValue);

