#ifndef AGENTS_OPTIONALRENDERINGS_H
#define AGENTS_OPTIONALRENDERINGS_H

#include <i3d/image3d.h>
#include "../../util/Vector3d.h"
#include "../../DisplayUnits/DisplayUnit.h"

/** renders (local) grid of lines that align with voxel centres,
	 that is, the boxes that are created do not represent individual voxels,
	 returns the number of lines it has created */
template <class P>
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
#endif
