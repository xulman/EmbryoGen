#include <list>
#include <i3d/image3d.h>
#include "report.h"
#include "Vector3d.h"
#include "../Geometries/Geometry.h"
#include "../Agents/AbstractAgent.h"
#include "AgentsMap.hpp"

	// ----------------- start of manipulators/managers -----------------
	/** give span and "resolution" as voxel size,
	    the final maxCorner might end up a bit outside from the original request
	    in order to host a _complete_ cells within the volume */
	void AgentsMap::reset(const VF& _minCorner,const VF& _maxCorner,const VF& _cellSize)
	{
		minCorner = _minCorner;
		maxCorner = _maxCorner;
		cellSize  = _cellSize;

		maxCorner -= minCorner;
		maxCorner.elemDivBy(cellSize);
		maxCorner.elemCeil();

		mapShape.from(maxCorner);

		maxCorner.elemMult(cellSize);
		maxCorner += minCorner;

		resizeMap();
	}

	//TODO
	/** give span and "resolution" as number of voxels */
	/*
	void reset(minCorner,maxCorner,mapShape)
	{
	}
	*/

	/** give position and "size" of the map */
	/*
	void reset(minCorner,cellSize,mapShape)
	{
	}
	*/

	/** give position and "size" of the map */
	/*
	void reset(centreOfTheMap,cellSize,mapShape)
	{
	}
	*/

	void AgentsMap::resizeMap()
	{
		map.clear();
		map.resize( mapShape.x * mapShape.y * mapShape.z );
		for (CellContainer& c : map) c.reserve(optimalCellCapacity);

		isUsableFlag = true;
	}

	void AgentsMap::clearMap()
	{
		for (auto& v : map) v.clear();
	}
	// ----------------- end of manipulators/managers -----------------

	// ----------------- start of users to the above group -----------------
	template <typename IntVT>
	void AgentsMap::printHeatMap(i3d::Image3d<IntVT>& heatMap)
	{
		//reset the heat image to map 1:1 onto this->map
		heatMap.MakeRoom( mapShape.toI3dVector3d() );
		heatMap.SetResolution(i3d::Resolution( i3d::Vector3d<float>(1.0f/cellSize.x,1.0f/cellSize.y,1.0f/cellSize.z) ));
		heatMap.SetOffset( minCorner.toI3dVector3d() );

		//just clone the sizes
		IntVT* pHM = heatMap.GetFirstVoxelAddr();
		for (size_t i = 0; i < heatMap.GetImageSize(); ++i, ++pHM) *pHM = (IntVT)map[i].size();
	}

	template void AgentsMap::printHeatMap(i3d::Image3d<i3d::GRAY8>& heatMap);
	template void AgentsMap::printHeatMap(i3d::Image3d<i3d::GRAY16>& heatMap);
	template void AgentsMap::printHeatMap(i3d::Image3d<size_t>& heatMap);

	void AgentsMap::printStats()
	{
		REPORT("min->maxCorner: " << minCorner << " -> " << maxCorner << ", cellSize=" << cellSize);
		REPORT("mapShape=" << mapShape << ", size (theoretically) "
		  << humanFriendlyNumber(mapShape.x*mapShape.y*mapShape.z * optimalCellCapacity * sizeof(int)) << "B");
	}


	/** appends content of the cell that contains 'aroundThisRealCoordinate' */
	void AgentsMap::getNearbyAgentIDs(const VF& aroundThisRealCoordinate,
	                       std::list<int>& nearbyIDs)
	{
		//size_t centreIndex = getCellIndex_fromRealCoords(aroundThisRealCoordinate);
		//for (int id : map[centreIndex]) nearbyIDs.push_back(id);
		//
		const CellContainer& cell = map[ getCellIndex_fromRealCoords(aroundThisRealCoordinate) ];
		nearbyIDs.insert(nearbyIDs.end(),cell.cbegin(),cell.cend());
	}

	/** appends content of the cells that are touched by sphere at 'aroundThisRealCoordinate'
	    with radius 'perimeterOfInterest' */
	void AgentsMap::getNearbyAgentIDs(const VF& aroundThisRealCoordinate,
	                       const G_FLOAT perimeterOfInterest,
	                       std::list<int>& nearbyIDs)
	{
		VI centrePos( getCellCoordinate(aroundThisRealCoordinate) );

		//determine how many cells to span to all sides
		VI halfSpan(
				(int)std::ceil( perimeterOfInterest / cellSize.x ),
				(int)std::ceil( perimeterOfInterest / cellSize.y ),
				(int)std::ceil( perimeterOfInterest / cellSize.z )
			);

		VI from(centrePos-halfSpan);
		from.elemMax(VI(0));

		VI till(centrePos+halfSpan);
		till.elemMin(mapShape-VI(1));

		//DEBUG_REPORT("halfSpan=" << halfSpan << ", minSweep=" << from << ", maxSweep=" << till);

		forCycle<int>(from,till,VI(1), [&nearbyIDs,this](const VI& mapPos)
		{
			//std::cout << "testing: " << mapPos << "\n";
			//size_t centreIndex = getCellIndex(mapPos);
			//for (int id : map[centreIndex]) nearbyIDs.push_back(id);
			//
			const CellContainer& cell = map[ getCellIndex(mapPos) ];
			nearbyIDs.insert(nearbyIDs.end(),cell.cbegin(),cell.cend());
		});
	}

	void AgentsMap::insertAgent(const AbstractAgent& ag)
	{
		insertAgent(ag.getID(),ag.getGeometry().AABB);
	}

	void AgentsMap::insertAgent(const int ID, const AxisAlignedBoundingBox& aabb)
	{
		//get centre of the AABB
		Vector3d<G_FLOAT> pos(aabb.minCorner);
		pos += aabb.maxCorner;
		pos /= 2.0f;

		insertAgent_atRealCoord(ID,pos);
	}

	void AgentsMap::insertAgent_atRealCoord(const int ID, const VF& realCoord)
	{
		getCellRef_fromRealCoords(realCoord).push_back(ID);
	}

	void AgentsMap::insertAgent_atRealCoord(const int ID,
	         const G_FLOAT xRealCoord, const G_FLOAT yRealCoord, const G_FLOAT zRealCoord)
	{
		getCellRef_fromRealCoords(xRealCoord,yRealCoord,zRealCoord).push_back(ID);
	}

	void AgentsMap::insertAgent_atMapCoord(const int ID, const VI& mapCoord)
	{
		getCellRef(mapCoord).push_back(ID);
	}

	void AgentsMap::insertAgent_atMapCoord(const int ID,
	         const int xMapCoord, const int yMapCoord, const int zMapCoord)
	{
		getCellRef(xMapCoord,yMapCoord,zMapCoord).push_back(ID);
	}
	// ----------------- end of users to the above group -----------------
