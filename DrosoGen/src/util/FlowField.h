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
	FlowField(FlowField<FT> const &FF) {
		if (FF.x) x=new i3d::Image3d<FT>(*FF.x); else x=NULL;
		if (FF.y) y=new i3d::Image3d<FT>(*FF.y); else y=NULL;
		if (FF.z) z=new i3d::Image3d<FT>(*FF.z); else z=NULL;
	}

	///destructor
	~FlowField() { if (x) delete x; if (y) delete y; if (z) delete z; }

	///the vector elements
	i3d::Image3d<FT> *x;
	i3d::Image3d<FT> *y;
	i3d::Image3d<FT> *z;

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
	bool isAligned(FlowField<FT> const &FF, const FT precision2=0.0001) {
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
};
#endif
