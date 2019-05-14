/**********************************************************************
*
* THIS CODE IS EXTRACTED FROM types.h
*
* The file 'types.h' is part of MitoGen
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


#ifndef MITOGEN_TYPES_H
#define MITOGEN_TYPES_H

#include <i3d/image3d.h>
#include <i3d/vector3d.h>
#include "report.h"
#include "../DisplayUnits/DisplayUnit.h"

/**
 * A wrapper for two or three images that should represent 2D or 3D vector
 * flow field.
 *
 * All images must be of the same size, resolution, and offset.
 * In the 2D case, the third \e z image is not be present, i.e. z==NULL.
 */
template <class FT>
struct FlowField {
public:
	///empty constructor
	FlowField() : x(NULL), y(NULL), z(NULL) {}

	///copy constructor: makes extra copies of flow fields
	FlowField(FlowField<FT> const &FF)
	{
		if (FF.x) x=new i3d::Image3d<FT>(*FF.x); else x=NULL;
		if (FF.y) y=new i3d::Image3d<FT>(*FF.y); else y=NULL;
		if (FF.z) z=new i3d::Image3d<FT>(*FF.z); else z=NULL;
	}

	///destructor
	~FlowField()
	{
		if (canDeleteXYZ)
		{
			if (x) delete x;
			if (y) delete y;
			if (z) delete z;
		}
	}

	///the vector elements
	i3d::Image3d<FT> *x;
	i3d::Image3d<FT> *y;
	i3d::Image3d<FT> *z;

	///flag to signal if underlying image containers can be freed
	bool canDeleteXYZ = true;

	///connect/overlay outside underlying image containers
	void proxify(i3d::Image3d<FT>* _x,i3d::Image3d<FT>* _y,i3d::Image3d<FT>* _z)
	{
		x = _x;  y = _y;  z = _z;
		canDeleteXYZ = false;
	}

	///intentionally lose connection with underlying image containers
	void deproxify(void)
	{
		x = y = z = NULL;
		canDeleteXYZ = true;
	}

	/**
	 * Is the flow field consistent? That is, \e x and \e y must exist
	 * and must be of the same size. If \e z exists as well, it must be
	 * also of the same size. If then \e x is 3D image, \e z must exists.
	 * Also, the resolution and offset of all images must be the same.
	 *
	 * \returns True, if consistent.
	 */
	bool isConsistent(void) const {
		if (!x) return false;
		if (!y) return false;
		if (x->GetSize() != y->GetSize()) return false;
		if (x->GetResolution().GetRes() != y->GetResolution().GetRes()) return false;
		if (x->GetOffset() != y->GetOffset()) return false;

		if ( (x->GetSizeZ() > 1) && (!z) ) return false;
		if (z) {
			if (x->GetSize() != z->GetSize()) return false;
			if (x->GetResolution().GetRes() != z->GetResolution().GetRes()) return false;
			if (x->GetOffset() != z->GetOffset()) return false;
		}
		return true;
	}

	/**
	 * Does the pixel grid of the flow field align with the given one? That is,
	 * resolution of this and \e FF must be the same and the difference of their
	 * offsets, which is a vector, must be, axes-wise, an integer multiple of pixels.
	 * The later test for "being an integer vector" is performed by element-wise
	 * subtracting roundf() of respective vector element and testing whether the
	 * squared size of the residual vector is smaller than \e precision2, which is
	 * again a squared value.
	 *
	 * \param[in] FF		a flow field to test alignment against
	 * \param[in] precision2	how far, in pixels, two grids can be offset
	 *
	 * \returns True, if considered aligned.
	 */
	bool isAligned(FlowField<FT> const &FF, const FT precision2=(FT)0.0001) {
		//does it have the same spacing of the pixel grid?
		if (x->GetResolution().GetRes() != FF.x->GetResolution().GetRes())
			return false;

		//turns offset [um] using resolution [px/um] into distance in pixels
		i3d::Vector3d<float> offDiff=x->GetOffset() - FF.x->GetOffset();
		const i3d::Vector3d<float> res=x->GetResolution().GetRes();
		//and subtract element-wise the integer parts
		//(we should obtain very close to integer numbers)
		offDiff.x *= res.x; offDiff.x-=roundf(offDiff.x);
		offDiff.y *= res.y; offDiff.y-=roundf(offDiff.y);
		offDiff.z *= res.z; offDiff.z-=roundf(offDiff.z);

		//is the shift of offsets a multiple of grid spacing?
		//(to some extent... bounded by the precision2)
		//DEBUG TODO VLADO remove this report, and #include"macros.h" above top
		DEBUG_REPORT("Sq. difference of grid offsets is "
			<< i3d::Norm2(offDiff) << " in pixels, allowed sq. precision is "
			<< precision2 << " in pixels");
		if (i3d::Norm2(offDiff) > precision2)
			return false;

		return true;
	}


	/** Draws content of this FF using the given display unit,
	    with the given color and with increasing IDs starting from
	    the one given. The number of drawn/created vectors is returned.
	    The 'sparsity' defines the sampling rate (how many pixels
	    to skip over) at which the FF is displayed. */
	int drawFF(DisplayUnit& du, const int ID, const int color,
	           const Vector3d<size_t>& sparsity) const
	{
		//return value
		int elemCnt = 0;

		//offset and resolution of the flow field images/containers
		const Vector3d<float> off(x->GetOffset());
		const Vector3d<float> res(x->GetResolution().GetRes());

		//any position in microns, and some vector
		Vector3d<float> pos,vec;

		//sweep the FF, voxel by voxel
		Vector3d<size_t> pxPos;
		for (pxPos.z = 0; pxPos.z < x->GetSizeZ(); pxPos.z += sparsity.z)
		for (pxPos.y = 0; pxPos.y < x->GetSizeY(); pxPos.y += sparsity.y)
		for (pxPos.x = 0; pxPos.x < x->GetSizeX(); pxPos.x += sparsity.x)
		{
			//translate px coord into micron (real world) one
			pos.toMicronsFrom(pxPos, res,off);

			vec.x = x->GetVoxel(pxPos.x, pxPos.y, pxPos.z);
			vec.y = y->GetVoxel(pxPos.x, pxPos.y, pxPos.z);
			vec.z = z->GetVoxel(pxPos.x, pxPos.y, pxPos.z);

			//display only non-zero vectors
			if (vec.len2() > 0)
				du.DrawVector(ID+ elemCnt++,pos,vec,color);
		}

		return elemCnt;
	}


	/** Draws content of this FF using the drawFF(...,sparsity)
	    with 'sparsity' = (1,1,1), that is, all vectors are displayed. */
	int drawFF(DisplayUnit& du, int ID, const int color) const
	{
		return drawFF(du,ID,color, Vector3d<size_t>(1));
	}
};
#endif
