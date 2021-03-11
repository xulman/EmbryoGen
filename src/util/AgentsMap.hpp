#ifndef AGENTSMAP_H
#define AGENTSMAP_H

#include <vector>
#include <list>
#include <i3d/image3d.h>
#include "Vector3d.h"
#include "../Geometries/Geometry.h"
class AbstractAgent;

class AgentsMap
{
public:
	typedef Vector3d<G_FLOAT> VF;
	typedef Vector3d<int>     VI;
	typedef std::vector<int>  CellContainer;

	// ----------------- start of the group of mutually involving map shapers/parameters -----------------
	/** min corner (as absolute coordinate) [in microns] of the map's span */
	VF minCorner;

	/** max corner (as absolute coordinate) [in microns] of the map's span */
	VF maxCorner;

	/** size [in microns] of one (volumetric) cell in the map, cell = voxel */
	VF cellSize;

	/** numbers of cells along x,y,z axes */
	VI mapShape;

	/** a buffer of the 'mapShape' that holds vector of agents IDs in every cell */
	std::vector< CellContainer > map;

	/** for how many agents shall the cells be designed to easily store,
	    this is not an upper bound -- cells can store more if needed, but
	    at some performance cost */
	int optimalCellCapacity = 100;
	// ----------------- end of the group of mutually involving map shapers/parameters -----------------

	// ----------------- start of position getters -----------------
	/** gets cell position as x,y,z from linear index/offset within the 'map' */
	VI getCellCoordinate(size_t index) const
	{
		VI pos;
		pos.z = (int)(index / (mapShape.x*mapShape.y)); //auto down-rounded
		index -= pos.z * mapShape.x*mapShape.y;
		pos.y = (int)(index / mapShape.x);              //auto down-rounded
		pos.x = (int)(index - pos.y*mapShape.x);
		return pos;
	}

	/** gets cell position as x,y,z from absolute real coordinate */
	VI getCellCoordinate(const G_FLOAT xRealCoord, const G_FLOAT yRealCoord, const G_FLOAT zRealCoord) const
	{
		VF pos(xRealCoord,yRealCoord,zRealCoord);
		pos -= minCorner;
		pos.elemDivBy(cellSize);
		pos.elemFloor();
		return pos.to<int>();
	}

	/** gets cell position as x,y,z from absolute real coordinate */
	VI getCellCoordinate(const VF& realCoord) const
	{
		return getCellCoordinate(realCoord.x,realCoord.y,realCoord.z);
	}


	/** gets cell position as linear coordinate/index/offset within the 'map' from x,y,z */
	size_t getCellIndex(const int xMapCoord, const int yMapCoord, const int zMapCoord) const
	{
		return (xMapCoord + mapShape.x*(yMapCoord + mapShape.y*zMapCoord));
	}

	/** gets cell position as linear coordinate/index/offset within the 'map' from x,y,z */
	size_t getCellIndex(const VI& mapCoord) const
	{
		return (mapCoord.x + mapShape.x*(mapCoord.y + mapShape.y*mapCoord.z));
	}


	/** gets cell position as linear coordinate/index/offset within the 'map' from absolute real coordinate */
	size_t getCellIndex_fromRealCoords(const G_FLOAT xRealCoord, const G_FLOAT yRealCoord, const G_FLOAT zRealCoord) const
	{
		return getCellIndex( getCellCoordinate(xRealCoord,yRealCoord,zRealCoord) );
	}

	/** gets cell position as linear coordinate/index/offset within the 'map' from absolute real coordinate */
	size_t getCellIndex_fromRealCoords(const VF& realCoord) const
	{
		return getCellIndex( getCellCoordinate(realCoord.x,realCoord.y,realCoord.z) );
	}


	CellContainer& getCellRef(const int xMapCoord, const int yMapCoord, const int zMapCoord)
	{
		//TODO: test span in debug! ...also in the methods below
		return map[ getCellIndex(xMapCoord,yMapCoord,zMapCoord) ];
	}

	CellContainer& getCellRef(const VI& mapCoord)
	{
		return map[ getCellIndex(mapCoord) ];
	}

	CellContainer& getCellRef_fromRealCoords(const G_FLOAT xRealCoord, const G_FLOAT yRealCoord, const G_FLOAT zRealCoord)
	{
		return map[ getCellIndex_fromRealCoords(xRealCoord,yRealCoord,zRealCoord) ];
	}

	CellContainer& getCellRef_fromRealCoords(const VF& realCoord)
	{
		return map[ getCellIndex_fromRealCoords(realCoord) ];
	}
	// ----------------- end of position getters -----------------

	// ----------------- start of manipulators/managers -----------------
protected:
	bool isUsableFlag = false;

public:
	bool isUsable()
	{ return isUsableFlag; }

	void stopUsing()
	{ isUsableFlag = false; }

	/** give span and "resolution" as voxel size,
	    the final maxCorner might end up a bit outside from the original request
	    in order to host a _complete_ cells within the volume */
	void reset(const VF& _minCorner,const VF& _maxCorner,const VF& _cellSize);

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

protected:
	void resizeMap();

public:
	void clearMap();
	// ----------------- end of manipulators/managers -----------------

	// ----------------- start of users to the above group -----------------
	template <typename IntVT>
	void printHeatMap(i3d::Image3d<IntVT>& heatMap);

	void printStats();


	/** appends content of the cell that contains 'aroundThisRealCoordinate' */
	void getNearbyAgentIDs(const VF& aroundThisRealCoordinate,
	                       std::list<int>& nearbyIDs);

	/** appends content of the cells that are touched by sphere at 'aroundThisRealCoordinate'
	    with radius 'perimeterOfInterest' */
	void getNearbyAgentIDs(const VF& aroundThisRealCoordinate, const G_FLOAT perimeterOfInterest,
	                       std::list<int>& nearbyIDs);

	void insertAgent(const AbstractAgent& ag);

	void insertAgent(const int ID, const AxisAlignedBoundingBox& aabb);

	void insertAgent_atRealCoord(const int ID, const VF& realCoord);

	void insertAgent_atRealCoord(const int ID,
	         const G_FLOAT xRealCoord, const G_FLOAT yRealCoord, const G_FLOAT zRealCoord);

	void insertAgent_atMapCoord(const int ID, const VI& mapCoord);

	void insertAgent_atMapCoord(const int ID,
	         const int xMapCoord, const int yMapCoord, const int zMapCoord);
	// ----------------- end of users to the above group -----------------
};
#endif
