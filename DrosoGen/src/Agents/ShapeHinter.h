#ifndef SHAPEHINTER_H
#define SHAPEHINTER_H

#include <list>
#include "../util/report.h"
#include "AbstractAgent.h"
#include "../Geometries/MaskImg.h"

class ShapeHinter: public AbstractAgent
{
public:
	ShapeHinter(const int ID, const MaskImg& shape,
	            const float currTime, const float incrTime)
		: AbstractAgent(ID,geometryAlias,currTime,incrTime),
		  geometryAlias(shape),
		  futureGeometry(shape)
	{
		//update AABBs
		geometryAlias.Geometry::setAABB();
		futureGeometry.Geometry::setAABB();

		DEBUG_REPORT("EmbryoShell with ID=" << ID << " was just created");
		DEBUG_REPORT("AABB: " << geometryAlias.AABB.minCorner << " -> " << geometryAlias.AABB.maxCorner);
	}

	~ShapeHinter(void)
	{
		DEBUG_REPORT("EmbryoShell with ID=" << ID << " was just deleted");
	}


private:
	// ------------- internals state -------------

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	MaskImg geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry, even the same noOfSpheres */
	MaskImg futureGeometry;

	// ------------- externals geometry -------------
	//FOR DEBUG ONLY!
	/** limiting distance beyond which I consider no interaction possible
	    with other nuclei */
	float ignoreDistance = 85.f;

	//FOR DEBUG ONLY!
	/** locations of possible interaction with nearby nuclei */
	std::list<ProximityPair> proximityPairs;

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override
	{

		//increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces(void) override
	{
		//update my futureGeometry
	}

	void collectExtForces(void) override
	{
		//FOR DEBUG ONLY!
		//scheduler, please give me ShadowAgents that are not further than ignoreDistance
		//(and the distance is evaluated based on distances of AABBs)
		std::list<const ShadowAgent*> l;
		Officer->getNearbyAgents(this,ignoreDistance,l);

		DEBUG_REPORT("Hinter: Found " << l.size() << " nearby agents");

		//those on the list are ShadowAgents who are potentially close enough
		//to interact with me and these I need to inspect closely
		proximityPairs.clear();
/*
		for (auto sa = l.begin(); sa != l.end(); ++sa)
			geometry.getDistance((*sa)->getGeometry(),proximityPairs);

*/
		//now, postprocess the proximityPairs
		DEBUG_REPORT("Hinter: Found " << proximityPairs.size() << " proximity pairs");
	}

	void adjustGeometryByExtForces(void) override
	{
		//update my futureGeometry
	}

	//futureGeometry -> geometryAlias
	void updateGeometry(void) override
	{
		//since we're not changing ShadowAgent::geometry (and consequently
		//not this.geometryAlias), we don't need to update this.futureGeometry
	}

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override
	{
		//draw bounding box of the GradIN region of the MaskImg
		int dID = ID << 17;
		dID += futureGeometry.AABB.drawIt(dID,1, du);

		//draw bounding box of the complete MaskImg, as a global debug element
		futureGeometry.AABB.drawBox(ID << 4,4,
		  futureGeometry.getDistImgOff(),futureGeometry.getDistImgFarEnd(), du);
	}

	void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override
	{
		//shortcuts to the mask image parameters
		const i3d::Vector3d<float>& res = img.GetResolution().GetRes();
		const Vector3d<FLOAT>       off(img.GetOffset().x,img.GetOffset().y,img.GetOffset().z);

		//shortcuts to our own geometry
		const i3d::Image3d<float>& distImg = futureGeometry.getDistImg();
		const Vector3d<FLOAT>&  distImgRes = futureGeometry.getDistImgRes();
		const Vector3d<FLOAT>&  distImgOff = futureGeometry.getDistImgOff();
		const MaskImg::DistanceModel model = futureGeometry.getDistImgModel();

		//project and "clip" this AABB into the img frame
		//so that voxels to sweep can be narrowed down...
		//
		//   sweeping position and boundaries (relevant to the 'img')
		Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
		futureGeometry.AABB.exportInPixelCoords(img, minSweepPX,maxSweepPX);
		//
		//micron coordinate of the running voxel 'curPos'
		Vector3d<FLOAT> centre;
		//
		//px coordinate of the voxel that is counterpart in distImg to the running voxel
		Vector3d<size_t> centrePX;

		//sweep within the intersection of the 'img' and futureGeometry::distImg
		for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
		for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
		for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
		{
			//get micron coordinate of the current voxel's centre
			//NB: AABB.exportInPixelCoords() assures that voxel centres fall into AABB
			centre.x = ((FLOAT)curPos.x +0.5f) / res.x;
			centre.y = ((FLOAT)curPos.y +0.5f) / res.y;
			centre.z = ((FLOAT)curPos.z +0.5f) / res.z;
			centre += off;

			//project the voxel's 'centre' to the futureGeometry.distImg
			centre -= distImgOff;
			centre.elemMult(distImgRes); //in px & in real coords

#ifdef DEBUG
			if (centre.x < 0 || centre.y < 0 || centre.z < 0
			 || centre.x < distImg.GetSizeX() || centre.y < distImg.GetSizeY() || centre.z < distImg.GetSizeZ())
				REPORT(ID << " gives counter-voxel " << centre << " outside of the distImg");
#endif

			//down-round to find the voxel that holds this (real)coordinate
			centrePX.x = (size_t)centre.x;
			centrePX.y = (size_t)centre.y;
			centrePX.z = (size_t)centre.z;
			//extract the value from the distImg
			const float dist = distImg.GetVoxel(centrePX.x,centrePX.y,centrePX.z);

			if (dist < 0 || (dist == 0 && model == MaskImg::DistanceModel::ZeroIN_GradOUT))
				img.SetVoxel(curPos.x,curPos.y,curPos.z, (i3d::GRAY16)ID);
				//NB: should dilate by 1px for model == GradIN_ZeroOUT
		}
	}
};
#endif
