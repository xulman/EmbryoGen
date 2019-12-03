#ifndef DISPLAYUNITS_UTIL_RENDERINGFUNCTIONS_H
#define DISPLAYUNITS_UTIL_RENDERINGFUNCTIONS_H

#include <i3d/image3d.h>
#include "../../util/Vector3d.h"
#include "../DisplayUnit.h"

class RenderingFunctions
{
public:
	/** Renders given bounding box into the given DisplayUnit 'du'
	    under the given 'ID' with the given 'color'. Multiple graphics
	    elements are required, so a couple of consecutive IDs are used
	    starting from the given 'ID'. Returned value tells how many
	    elements were finally used. */
	template <typename FLOAT>
	static
	int drawBox(DisplayUnit& du,
	            const int ID, const int color,
	            const Vector3d<FLOAT> &minC,
	            const Vector3d<FLOAT> &maxC)
	{
		//horizontal lines
		du.DrawLine( ID+0,
		  minC,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  minC.z),color );

		du.DrawLine( ID+1,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  minC.z),
		  Vector3d<float>(maxC.x,
		                  maxC.y,
		                  minC.z),color );

		du.DrawLine( ID+2,
		  Vector3d<float>(minC.x,
		                  minC.y,
		                  maxC.z),
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  maxC.z),color );

		du.DrawLine( ID+3,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  maxC.z),
		  maxC,color );

		//vertical lines
		du.DrawLine( ID+4,
		  minC,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  minC.z),color );

		du.DrawLine( ID+5,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  minC.z),
		  Vector3d<float>(maxC.x,
		                  maxC.y,
		                  minC.z),color );

		du.DrawLine( ID+6,
		  Vector3d<float>(minC.x,
		                  minC.y,
		                  maxC.z),
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  maxC.z),color );

		du.DrawLine( ID+7,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  maxC.z),
		  maxC,color );

		//"axial" lines
		du.DrawLine( ID+8,
		  minC,
		  Vector3d<float>(minC.x,
		                  minC.y,
		                  maxC.z),color );

		du.DrawLine( ID+9,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  minC.z),
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  maxC.z),color );

		du.DrawLine( ID+10,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  minC.z),
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  maxC.z),color );

		du.DrawLine( ID+11,
		  Vector3d<float>(maxC.x,
		                  maxC.y,
		                  minC.z),
		  maxC,color );

		return 12;
	}


	/** renders (local) grid of lines that align with voxel centres,
	    that is, the boxes that are created do not represent individual voxels,
	    returns the number of lines it has created */
	template <class P>
	static
	int drawPixelCentresGrid(DisplayUnit& du, const int ID, const int color,
	                         const i3d::Image3d<P>& refImg,
	                         const Vector3d<size_t> centrePx,
	                         const Vector3d<size_t> spanPx)
	{
		//return value
		int elemCnt = 0;

		//shortcuts to our own geometry
		const Vector3d<float> res( refImg.GetResolution().GetRes() );
		const Vector3d<float> off( refImg.GetOffset() );

		//pixel and micron coordinates
		Vector3d<size_t> curPos;
		Vector3d<float> a,b;

		//rays through pixel centres "from the left"
		for (curPos.z = centrePx.z-spanPx.z; curPos.z <= centrePx.z+spanPx.z; curPos.z++)
		for (curPos.y = centrePx.y-spanPx.y; curPos.y <= centrePx.y+spanPx.y; curPos.y++)
		{
			curPos.x = centrePx.x-spanPx.x;
			a.toMicronsFrom(curPos, res,off);

			curPos.x = centrePx.x+spanPx.x;
			b.toMicronsFrom(curPos, res,off);

			du.DrawLine(ID+ elemCnt++, a,b, color);
		}

		//rays "from the front"
		for (curPos.z = centrePx.z-spanPx.z; curPos.z <= centrePx.z+spanPx.z; curPos.z++)
		for (curPos.x = centrePx.x-spanPx.x; curPos.x <= centrePx.x+spanPx.x; curPos.x++)
		{
			curPos.y = centrePx.y-spanPx.y;
			a.toMicronsFrom(curPos, res,off);

			curPos.y = centrePx.y+spanPx.y;
			b.toMicronsFrom(curPos, res,off);

			du.DrawLine(ID+ elemCnt++, a,b, color);
		}

		//rays "from the bottom"
		for (curPos.y = centrePx.y-spanPx.y; curPos.y <= centrePx.y+spanPx.y; curPos.y++)
		for (curPos.x = centrePx.x-spanPx.x; curPos.x <= centrePx.x+spanPx.x; curPos.x++)
		{
			curPos.z = centrePx.z-spanPx.z;
			a.toMicronsFrom(curPos, res,off);

			curPos.z = centrePx.z+spanPx.z;
			b.toMicronsFrom(curPos, res,off);

			du.DrawLine(ID+ elemCnt++, a,b, color);
		}

		return elemCnt;
	}
};
#endif
