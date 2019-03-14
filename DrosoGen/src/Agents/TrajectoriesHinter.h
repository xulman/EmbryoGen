#ifndef TRAJECTORIESHINTER_H
#define TRAJECTORIESHINTER_H

#include "../util/report.h"
#include "AbstractAgent.h"
#include "../Geometries/VectorImg.h"
#include "../TrackRecord.h"
#include "../util/flowfields.h"
#include <i3d/morphology.h>

class TrajectoriesHinter: public AbstractAgent
{
public:
	template <class T>
	TrajectoriesHinter(const int ID, const std::string& type,
	                   const i3d::Image3d<T>& templateImg,
	                   const VectorImg::ChoosingPolicy policy,
	                   const float currTime, const float incrTime)
		: AbstractAgent(ID,type, geometryAlias, currTime,incrTime),
		  geometryAlias(templateImg,policy)
	{
		//update AABBs
		geometryAlias.Geometry::updateOwnAABB();
		geometryAlias.proxifyFF(ff);
		lastUpdatedTime = currTime-1;

		DEBUG_REPORT("EmbryoTraces with ID=" << ID << " was just created");
		DEBUG_REPORT("AABB: " << geometryAlias.AABB.minCorner << " -> " << geometryAlias.AABB.maxCorner);
	}

	~TrajectoriesHinter(void)
	{
		DEBUG_REPORT("EmbryoTraces with ID=" << ID << " was just deleted");
	}

	TrackRecords& talkToHinter()
	{
		return traHinter;
	}


private:
	// ------------- internals state -------------
	/** manager of the trajectories of all tracks */
	TrackRecords traHinter;

	/** binding FF between traHinter and geometryAlias */
	FlowField<float> ff;

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	VectorImg geometryAlias;
	float lastUpdatedTime;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry */
	//VectorImg futureGeometry;

	// ------------- externals geometry -------------

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override
	{
		//increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces(void) override
	{
		//would update my futureGeometry
	}

	void collectExtForces(void) override
	{
		//hinter is not responding to its surrounding
	}

	void adjustGeometryByExtForces(void) override
	{
		//would update my futureGeometry
	}

	//futureGeometry -> geometryAlias
	void updateGeometry(void) override
	{
		if (currTime > lastUpdatedTime)
		{
			DEBUG_REPORT("updating FF from " << currTime-incrTime << " to " << currTime);

			//update the geometryAlias according to the currTime
			traHinter.resetToFF(currTime-incrTime,currTime, ff, Vector3d<float>(10.0f));
			lastUpdatedTime = currTime;
		}
		else
			DEBUG_REPORT("skipping update now");
	}

	// ------------- rendering -------------
	//to make sure the paths are not overwritten, we remember here their IDs span
	int didDrawTheWholePaths_IDspan = 0;

	void drawMask(DisplayUnit& du) override
	{
		if (didDrawTheWholePaths_IDspan == 0)
		{
			//scan all time points to read out every tracks' "bending corners"
			std::map< float,std::map< int,Coord3d<float> > >::const_iterator it;

			//the current "segment" of the currently rendered trajectory
			const Coord3d<float> *a,*b;

			//draw all paths
			for (int tID : traHinter.knownTracks)
			{
				//init the drawing... (by flagging the first node is missing)
				a = NULL;

				//scan all time points
				it = traHinter.begin();
				while (it != traHinter.end())
				{
					//does given timepoint contain info about our track?
					if (it->second.find(tID) != it->second.end())
					{
						//update and draw then...
						b = &(it->second.at(tID));

						if (a != NULL)
							du.DrawLine(didDrawTheWholePaths_IDspan++, *a,*b, 5);
						a = b;
					}

					++it;
				}
			}

			DEBUG_REPORT("drew all trajectories with " << didDrawTheWholePaths_IDspan << " lines");
		}

		//update the trajectory positioners (small balls)
		int ID = didDrawTheWholePaths_IDspan;
		for (int t : traHinter.knownTracks)
		{
			Coord3d<float> pos;
			if (traHinter.getPositionAlongTrack(currTime,t,pos))
				du.DrawPoint(ID,pos,2.0f,5);

			++ID;
		}

		ff.DrawFF(du,ID,6,Vector3d<size_t>(2));

		/*
		//reduce flow field to the boundary of the shape
		i3d::Image3d<i3d::GRAY8> initShapeEroded;
		i3d::ErosionO(initShape,initShapeEroded,1);

		i3d::GRAY8* i = initShape.GetFirstVoxelAddr();
		i3d::GRAY8* e = initShapeEroded.GetFirstVoxelAddr();
		float*      x = ff.x->GetFirstVoxelAddr();
		float*      y = ff.y->GetFirstVoxelAddr();
		float*      z = ff.z->GetFirstVoxelAddr();
		for (size_t qq=0; qq < initShape.GetImageSize(); ++qq)
		{
			if (*i == 0 || (*i > 0 && *e > 0)) { *x = *y = *z = 0; }
			++i; ++e;
			++x; ++y; ++z;
		}
		DEBUG_REPORT("printed " << ff.DrawFF(displayUnit,100,2,Vector3d<size_t>(4,2,2))-100 << " vectors");
		*/
	}

	/*
	void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override
	{
		//running pointers...
		const FLOAT* x = geometryAlias.getImgX().GetFirstVoxelAddr();     //input vector elements
		const FLOAT* y = geometryAlias.getImgY().GetFirstVoxelAddr();
		const FLOAT* z = geometryAlias.getImgZ().GetFirstVoxelAddr();
		i3d::GRAY16* m = img.GetFirstVoxelAddr();         //for vector magnitude
		i3d::GRAY16* const mE = m + img.GetImageSize();

		//sweep the output image
		while (m != mE)
		{
			//TODO: must translate coordinates img -> X,Y,Z to fetch the values
			*m = std::sqrt( (*x * *x) + (*y * *y) + (*z * *z) );
			++m; ++x; ++y; ++z;
		}
	}
	*/
};
#endif
