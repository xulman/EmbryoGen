#include <iostream>
#include "../util/Vector3d.h"

#define DEBUG
#include "../util/AgentsMap.hpp"

void reportMap(AgentsMap& m)
{

	std::cout << "elems: " << m.map.size() << "\n";
	std::cout << "minCorner: " << m.minCorner << "\n";
	std::cout << "maxCorner: " << m.maxCorner << "\n";

	std::cout << "int coord  : " << m.getCellIndex(1,2,3) << "\n";
	std::cout << "int coord  : " << m.getCellIndex(AgentsMap::VI(1,2,3)) << "\n";
	std::cout << "inhabitants: " << m.getCellRef(1,2,3).size() << "\n";

	std::cout << "int coord  : " << m.getCellCoordinate(115.0,125.0,171.0) << "\n";
	std::cout << "int coord  : " << m.getCellIndex_fromRealCoords(115.0,125.0,171.0) << "\n";
	std::cout << "inhabitants: " << m.getCellRef_fromRealCoords(115.0,125.0,171.0).size() << "\n";
	std::cout << "inhabitants: " << m.getCellRef_fromRealCoords(123.0,125.0,171.0).size() << "\n";
	std::cout << "---------------------------------\n";
}

void reportIDs(const std::vector<int>& v)
{
	for (int i : v) std::cout << i << ",";
	std::cout << " (vec)" << std::endl;
}

void reportIDs(const std::list<int>& l)
{
	for (int i : l) std::cout << i << ",";
	std::cout << " (list)" << std::endl;
}


int main(void)
{
	AgentsMap m;

	AgentsMap::VF minCorner(100);
	AgentsMap::VF maxCorner(195);
	AgentsMap::VF cellSize(10,10,20);
	m.reset(minCorner,maxCorner,cellSize);

	reportMap(m);

	m.insertAgent_atMapCoord(10, 1,2,3);
	m.insertAgent_atRealCoord(20, 116,124,172);
	m.insertAgent_atRealCoord(25, 126,124,172);

	reportMap(m);

	/*
	i3d::Image3d<i3d::GRAY8> heat;
	m.printHeatMap(heat);
	heat.SaveImage("h.tif");
	*/

	reportIDs( m.map[ m.getCellIndex(1,2,3) ] );
	reportIDs( m.map[ m.getCellIndex(2,2,3) ] );
	reportIDs( m.map[ m.getCellIndex(3,2,3) ] );

	std::list<int> agents;
	m.getNearbyAgentIDs( Vector3d<float>(113,123,171), agents);
	reportIDs( agents );
	m.getNearbyAgentIDs( Vector3d<float>(123,123,171), agents);
	reportIDs( agents );

	agents.clear();

	m.getNearbyAgentIDs( Vector3d<float>(113,123,171), 45, agents);
	reportIDs( agents );

	return 0;
}
