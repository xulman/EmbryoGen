#include "TrajectoriesHinter.h"

//#define DRAW_VOXEL_GRID_AROUND_TRAJECTORIES

void TrajectoriesHinter::drawForDebug(DisplayUnit& du)
{
	if (detailedDrawingMode)
	{
		const int DBG = DisplayUnit::firstIdForAgentDebugObjects(ID);
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
void TrajectoriesHinter::drawForDebug(i3d::Image3d<i3d::GRAY16>& img)
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
