#ifndef TRAJECTORIESHINTER_H
#define TRAJECTORIESHINTER_H

#include <i3d/morphology.h>
#include "../util/report.h"
#include "AbstractAgent.h"
#include "../Geometries/VectorImg.h"
#include "../TrackRecord.h"
#include "../util/flowfields.h"
#include "../DisplayUnits/util/RenderingFunctions.h"

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

		DEBUG_REPORT("EmbryoTracks with ID=" << ID << " was just created");
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
	void publishGeometry(void) override
	{
		if (currTime > lastUpdatedTime)
		{
			DEBUG_REPORT(IDSIGN << "updating FF from " << currTime-incrTime << " to " << currTime);

			//update the geometryAlias according to the currTime
			traHinter.resetToFF(currTime-incrTime,currTime, ff, Vector3d<float>(5.0f));
			lastUpdatedTime = currTime;
		}
		else
		{
			DEBUG_REPORT(IDSIGN << "skipping update now");
		}
	}

	// ------------- rendering -------------
//#define DRAW_VOXEL_GRID_AROUND_TRAJECTORIES

	void drawForDebug(DisplayUnit& du) override
	{
		if (detailedDrawingMode)
		{
			const int DBG = ID << 17 | 1 << 16;
			int createdLines = 0;
			int createdBalls = 0;
			int createdVecs  = 0;
			//NB: the same starting number/element_ID does not matter as the
			//    corresponding objects shall live in different vizu categories

#ifdef DRAW_VOXEL_GRID_AROUND_TRAJECTORIES
			Vector3d<size_t> centrePx;
			const Vector3d<float> res( geometryAlias.getImgX().GetResolution().GetRes() );
			const Vector3d<float> off( geometryAlias.getImgX().GetOffset() );
#endif

			//scan all time points to read out every tracks' "bending corners"
			std::map< float,std::map< int,Coord3d<float> > >::const_iterator it;

			//the current "segment" of the currently rendered trajectory
			const Coord3d<float> *a,*b;
			Coord3d<float> pos;

			//draw all paths that are still relevant to the current time
			for (int tID : traHinter.knownTracks)
			if (traHinter.getPositionAlongTrack(currTime,tID,pos))
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
							du.DrawLine(DBG+ createdLines++, *a,*b, 5);
						a = b;
					}

					++it;
				}

				//update the trajectory positioners (small balls)
				du.DrawPoint(DBG+ createdBalls++, pos,1.5f, 5);

#ifdef DRAW_VOXEL_GRID_AROUND_TRAJECTORIES
				//also draw a (local debug) pixel grid around the ball centre
				createdLines += RenderingFunctions::drawPixelCentresGrid(du,
				  DBG+createdLines, 1, geometryAlias.getImgX(),
				  pos.toPixels(centrePx, res,off), Vector3d<size_t>(3));
#endif
			}
			DEBUG_REPORT(IDSIGN << "trajectories: " << createdLines
			             << " lines and " << createdBalls << " balls");

			//render the current flow field
			createdVecs += ff.drawFF(du, DBG+createdVecs, 6, Vector3d<size_t>(2));
			DEBUG_REPORT(IDSIGN << "trajectories: " << createdVecs << " vectors making up tracks-induced-FF");
		}
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
