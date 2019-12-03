#include "../util/report.h"
#include "../DisplayUnits/util/RenderingFunctions.h"
#include "../util/surfacesamplers.h"
#include "ShapeHinter.h"

void ShapeHinter::drawForDebug(DisplayUnit& du)
{
	if (detailedDrawingMode)
	{
		int dID = ID << 17 | 1 << 16; //enable debug bit

		//draw bounding box of the complete ScalarImg
		dID += RenderingFunctions::drawBox(du, dID,4,
		  geometryAlias.getDistImgOff(),geometryAlias.getDistImgFarEnd());

		//render spheres along a certain isoline
		ImageSampler<float,float> is;
		Vector3d<float> periPoint;
		int periPointCnt=0;

		is.resetByMicronStep(geometryAlias.getDistImg(),
		                     [](const float px){ return px == 2; },
		                     Vector3d<float>(10,5,5));
		while (is.next(periPoint))
		{
			du.DrawPoint(dID++, periPoint, 0.3f, 4);
			++periPointCnt;
		}
		DEBUG_REPORT(IDSIGN << "surface consists of " << periPointCnt << " spheres");
	}
}


void ShapeHinter::drawForDebug(i3d::Image3d<i3d::GRAY16>& img)
{
	//shortcuts to the mask image parameters
	const Vector3d<FLOAT> res(img.GetResolution().GetRes());
	const Vector3d<FLOAT> off(img.GetOffset().x,img.GetOffset().y,img.GetOffset().z);

	//shortcuts to our own geometry
	const i3d::Image3d<float>&   distImg = geometryAlias.getDistImg();
	const Vector3d<FLOAT>&    distImgRes = geometryAlias.getDistImgRes();
	const Vector3d<FLOAT>&    distImgOff = geometryAlias.getDistImgOff();
	const ScalarImg::DistanceModel model = geometryAlias.getDistImgModel();

	//project and "clip" this AABB into the img frame
	//so that voxels to sweep can be narrowed down...
	//
	//   sweeping position and boundaries (relevant to the 'img')
	Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
	geometryAlias.AABB.exportInPixelCoords(img, minSweepPX,maxSweepPX);
	//
	//micron coordinate of the running voxel 'curPos'
	Vector3d<FLOAT> centre;
	//
	//px coordinate of the voxel that is counterpart in distImg to the running voxel
	Vector3d<size_t> centrePX;

	//sweep within the intersection of the 'img' and geometryAlias::distImg
	for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
	for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
	for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
	{
		//get micron coordinate of the current voxel's centre
		//NB: AABB.exportInPixelCoords() assures that voxel centres fall into AABB
		centre.toMicronsFrom(curPos, res,off);

		//project the voxel's 'centre' to the geometryAlias.distImg
		centre.fromMicronsTo(centrePX, distImgRes,distImgOff);

#ifdef DEBUG
		if (centrePX.x >= distImg.GetSizeX() || centrePX.y >= distImg.GetSizeY() || centrePX.z >= distImg.GetSizeZ())
			REPORT(IDSIGN << " gives counter-voxel " << centrePX << " (to voxel " << curPos << ") outside of the distImg");
#endif

		//extract the value from the distImg
		const float dist = distImg.GetVoxel(centrePX.x,centrePX.y,centrePX.z);

		if (dist < 0 || (dist == 0 && model == ScalarImg::DistanceModel::ZeroIN_GradOUT))
		{
#ifdef DEBUG
			i3d::GRAY16 val = img.GetVoxel(curPos.x,curPos.y,curPos.z);
			if (val > 0 && val != (i3d::GRAY16)ID)
				REPORT(IDSIGN << " overwrites mask of " << val << " at " << curPos);
#endif
			img.SetVoxel(curPos.x,curPos.y,curPos.z, (i3d::GRAY16)ID);
			//NB: should dilate by 1px for model == GradIN_ZeroOUT
		}
	}
}
