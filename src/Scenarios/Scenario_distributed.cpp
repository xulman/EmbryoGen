#include <thread>
#include <functional>
#include <i3d/image3d.h>
#include "../util/rnd_generators.h"
#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "common/Scenarios.h"

// ------------------ grid placement and related stuff ------------------
//
const float placementStepX = 10.f;
const float placementStepY = 10.f;
const float agentRadius = 3.f;     //must hold: placementStep[XY] > 2*agentRadius

float getDiagonalAgentsAABBdistance(const float dx, const float dy, const float r)
{
	//corners distance
	float rdx = dx - 2*r;
	float rdy = dy - 2*r;

	return std::sqrt(rdx*rdx + rdy*rdy);
}


// ------------------ work-pretending agent ------------------
//
/** empty agent that only randomly long waits (simulating some work),
    slowly translates, fills its pixel and reports its neighbors */
class ParallelNucleus //: public AbstractAgent
{
};


// ------------------ setting up the simulation scenario ------------------
//
SceneControls& Scenario_Distributed::provideSceneControls()
{
	SceneControls::Constants myConstants;
	//
	//do 20 (internal) iterations and then stop
	myConstants.stopTime = myConstants.initTime + 20*myConstants.incrTime;
	//
	//ask the system to report after every (internal) simulation step
	//myConstants.expoTime = myConstants.incrTime;

	return *(new SceneControls(myConstants));
}

void Scenario_Distributed::initializeScene()
{
	DEBUG_REPORT("enabling some image outputs...");

	//scenario has two optional params: how many agents along x and y axes
	const int howManyAlongX = argc > 2? atoi(argv[2]) : 5;
	const int howManyAlongY = argc > 3? atoi(argv[3]) : 4;

	//setup the output images: that many pixels as many agents
	params.setOutputImgSpecs(Vector3d<float>(0),    //offset: um
	                  Vector3d<float>((float)howManyAlongX,(float)howManyAlongY,1.f),    //size: um = px
	                  Vector3d<float>(1));   //resolution: px/um
	params.enableProducingOutput(params.imgPhantom);
	params.enableProducingOutput(params.imgOptics);
	params.enableProducingOutput(params.imgMask); //enable if you want to see the constellation of IDs
}

void Scenario_Distributed::initializeAgents(FrontOfficer* fo,int p,int P)
{
	//scenario has two optional params: how many agents along x and y axes
	const int howManyAlongX = argc > 2? atoi(argv[2]) : 5;
	const int howManyAlongY = argc > 3? atoi(argv[3]) : 4;

	//corner of the grid of agents such that centre of the grid coincides with scene's centre
	Vector3d<float> simCorner(params.constants.sceneSize);
	simCorner /= 2.0f;
	simCorner += params.constants.sceneOffset;
	simCorner.x -= placementStepX * ((float)howManyAlongX-1.f)/2.f;
	simCorner.y -= placementStepY * ((float)howManyAlongY-1.f)/2.f;

	//agents' metadata
	char agentName[512];
	int ID = 1;

	for (int y = 0; y < howManyAlongY; ++y)
	for (int x = 0; x < howManyAlongX; ++x)
	{
		//the wished position
		Vector3d<float> pos(simCorner);
		pos.x += placementStepX * (float)x;
		pos.y += placementStepY * (float)y;

		//shape (and placement)
		Spheres s(1);
		s.updateCentre(0,pos);
		s.updateRadius(0,agentRadius);

		//name
		sprintf(agentName,"nucleus %d @ %d,%d",ID,x,y);

		//ParallelNucleus* ag = new ParallelNucleus(ID,std::string(agentName),s,x,y,currTime,incrTime);
		//restoreMe//fo->startNewAgent(ag);

		++ID;
	}
}
