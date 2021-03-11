#include <thread>
#include <functional>
#include <i3d/image3d.h>
#include "common/Scenarios.h"
#include "../DisplayUnits/ConsoleDisplayUnit.h"
#include "../DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "../DisplayUnits/FileDisplayUnit.h"
#include "../DisplayUnits/FlightRecorderDisplayUnit.h"
#include "../util/rnd_generators.h"
#include "../util/Vector3d.h"
#include "../Geometries/Spheres.h"
#include "../Agents/AbstractAgent.h"
#include "../util/AgentsMap.hpp"

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
class ParallelNucleus: public AbstractAgent
{
public:
	ParallelNucleus(const int _ID, const std::string& _type,
	                const Spheres& shape, const int _x, const int _y,
	                const float _currTime, const float _incrTime):
	  AbstractAgent(_ID,_type,geometry,_currTime,_incrTime),
	  geometry(shape), x(_x), y(_y),
	  geometryDx( placementStepX/5.f,0.f,0.f ),
	  searchAroundDistance( 0.95f * getDiagonalAgentsAABBdistance(placementStepX,placementStepY,agentRadius) )
	{
		geometry.Geometry::updateOwnAABB();
#ifndef DEBUG
		REPORT(IDSIGN << "created: \"" << _type << "\"");
#endif
	}

	///local (and the only storage) of the current agent's geometry
	Spheres geometry;

	///relative position on the grid of nuclei
	int x,y;

	///flag to tell publishGeometry() to update the geometry (to prevent it from translating twice)
	bool shouldUpdateGeometry = false;

	///how much to translate the geometry
	const Vector3d<float> geometryDx;

	///how much to look around to find AABBs of nearby agents
	const float searchAroundDistance;

	///who has been found around
	std::list<const ShadowAgent*> nearbyAgents;


	/// internal affairs: flag that the agent should move
	void advanceAndBuildIntForces(const float)
	{
		//random duration (in full 2-10 seconds) pause here to pretend "some work"
		/*
		const int waitingTime = (int)GetRandomUniform(0,20);
		REPORT(IDSIGN << "pretends work that would last for " << waitingTime << " milisecond(s)");
		std::this_thread::sleep_for(std::chrono::milliseconds( (long long)waitingTime ));
		*/

		//ask to have the geometry updated as a result of the "some work"
		shouldUpdateGeometry = true;

		//increase the local time of the agent
		currTime += incrTime;
	}

	/// external affairs: just look and remember who is around
	void collectExtForces(void)
	{
		//reset a list of nearby agents
		nearbyAgents.clear();

		//ask scheduler to have it filled,
		//that is to record nearby agents
		Officer->getNearbyAgents(this,searchAroundDistance, nearbyAgents);
	}


	//we don't utilize any extra (2nd) geometry to be updated, and we cannot update
	//the official this.geometry here (that can happen only in publishGeometry()),
	//so these methods are empty
	void adjustGeometryByIntForces(void) {}
	void adjustGeometryByExtForces(void) {}

	void publishGeometry(void)
	{
		//some agents fail to update their geometry for some time
		if (shouldUpdateGeometry && x%18 == 17 && y%18 == 17 && currTime > 0.2 && currTime < 1.9)
		{
			//DEBUG_REPORT(SIGN << "failed to provide updated geometry");
			shouldUpdateGeometry = false;
			return;
		}

		if (shouldUpdateGeometry)
		{
			//translate agent a bit to the right
			geometry.updateCentre(0, geometry.getCentres()[0] + geometryDx);
			geometry.Geometry::updateOwnAABB();
			shouldUpdateGeometry = false;
		}
	}


	void drawMask(DisplayUnit& du)
	{
		//draw sphere at its current position, with the color of the responsible Officer
		du.DrawPoint(DisplayUnit::firstIdForAgentObjects(ID), geometry.getCentres()[0], geometry.getRadii()[0], Officer->getID());
	}

	void drawMask(i3d::Image3d<i3d::GRAY16>& mask)
	{
		//set my "grid pixel" to mark I was alive
		mask.SetVoxel((size_t)x,(size_t)y,0, (i3d::GRAY16)ID);
	}

	void drawTexture(i3d::Image3d<float>& phantom, i3d::Image3d<float>& indices)
	{
		//report the current time into the phantom
		phantom.SetVoxel((size_t)x,(size_t)y,0, currTime);

		//if this call is the very first snapshot (the one after the scene is build,
		//and no simulation round has been executed so far), the neighbors have not yet examined
		if (currTime == 0) collectExtForces();

		//report the encoded list of IDs of nearby agents with the purpose to spot
		//if the constellation around this agent has changed... (as a result of
		//badly updated info about surrounding bboxes)
		//
		//how to encode n IDs into scalar? f(a1,...,an) =
		// = sum_i ai -> no, different agents can easily give the same encoding, e.g. 19+20 = 18+21 
		// = prod_i ai -> no, with large grids (with large ID numbers) the product may not fit into float
		// = sum_i hash(ai) -> yes, provided n times hashDomainSizeInBits fits into float
		//                          (hash(19) != hash(20) != hash(18) != hash(21) and
		//                           also very unlikely hash(19)+hash(20) == hash(18)+hash(21))
		size_t hash = 0;
		for (const auto sa : nearbyAgents)
		{
			//get ID out of the sa->getAgentType()
			std::istringstream iss(sa->getAgentType());
			std::string ignore;
			int ID;
			iss >> ignore >> ID;
			//if (nearbyAgents.size() > 4) REPORT(SIGN << "sees around agent ID " << ID);
			hash += std::hash<int>()(ID);
		}
		indices.SetVoxel((size_t)x,(size_t)y,0, (float)hash);
	}
};


// ------------------ setting up the simulation scenario ------------------
//
SceneControls& Scenario_Parallel::provideSceneControls()
{
	SceneControls::Constants myConstants;
	//
	//do 20 (internal) iterations and then stop
	myConstants.stopTime = myConstants.initTime + 20*myConstants.incrTime;
	//
	//ask the system to report after every (internal) simulation step
	myConstants.expoTime = myConstants.incrTime;

	myConstants.imgPhantom_filenameTemplate = "timeStamps_%03u.tif";
	myConstants.imgOptics_filenameTemplate  = "neighbors_%03u.tif";


	class mySceneControl: public SceneControls
	{
	public:
		mySceneControl(Constants& c): SceneControls(c)
		{
			//DisplayUnits handling: variant A
			//displayUnit.RegisterUnit( myDU );
		}

		void updateControls(const float currTime) override
		{
			if (currTime == 0.1f)
			{
				DEBUG_REPORT("stopping the production of the maskXXX.tif files");
				ctx().disks.disableImgMaskTIFFs();
				//ctx().displays.unregisterImagingUnit("localFiji");
			}

			//DisplayUnits handling: variant A
			if (currTime == 0.3f)
			{
				DEBUG_REPORT("stopping the console reports");
				//displayUnit.UnregisterUnit( myDU );
			}
		}

		//DisplayUnits handling: variant A
		ConsoleDisplayUnit myDU;
	};

	mySceneControl* ctrl = new mySceneControl(myConstants);

	//DisplayUnits handling: variant B
	//
	//ctrl->displayUnit.RegisterUnit( new FileDisplayUnit("debugLog.txt") );
	//ctrl->displayUnit.RegisterUnit( new FlightRecorderDisplayUnit("FlightRecording.txt") );
	//ctrl->displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("localhost:8765") );
	//ctrl->displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("10.1.202.7:8765") );     //laptop @ Vlado's office
	//ctrl->displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("192.168.3.110:8765") );  //PC     @ Vlado's home

	return *ctrl;
}


void Scenario_Parallel::initializeScene()
{
	DEBUG_REPORT("enabling some image outputs...");

	//scenario has two optional params: how many agents along x and y axes
	const int howManyAlongX = argc > 2? atoi(argv[2]) : 5;
	const int howManyAlongY = argc > 3? atoi(argv[3]) : 4;

	//setup the output images: that many pixels as many agents
	params.setOutputImgSpecs(Vector3d<float>(0),    //offset: um
	                  Vector3d<float>((float)howManyAlongX,(float)howManyAlongY,1.f),    //size: um = px
	                  Vector3d<float>(1));   //resolution: px/um
	disks.enableImgPhantomTIFFs();
	disks.enableImgOpticsTIFFs();
	disks.enableImgMaskTIFFs(); //enable if you want to see the constellation of IDs

	//NB: ConsoleDisplay-ing is managed separately in provideSceneControls()
	//NB: Scenery/SimViewer-ing is managed separately in initializeAgents()

	//displays.registerImagingUnit("localFiji","localhost:54545");
	//displays.enableImgOpticsInImagingUnit("localFiji");
}


void Scenario_Parallel::initializeAgents(FrontOfficer* fo,int p,int P)
{
	static char url[32];
	sprintf(url,"192.168.3.105:%d",8764+p);
	//sprintf(url,"127.0.0.1:%d",8764+p);
	REPORT("Parallel Scenario: going to connect to scenery/SimViewer at " << url);
	displays.registerDisplayUnit( [](){ return new SceneryBufferedDisplayUnit(url); } );

	REPORT("Parallel Scenario p=" <<  p << " P=" << P<< " initializing now...");
	//scenario has two optional params: how many agents along x and y axes
	const int howManyAlongX = argc > 2? atoi(argv[2]) : 5;
	const int howManyAlongY = argc > 3? atoi(argv[3]) : 4;

	//corner of the grid of agents such that centre of the grid coincides with scene's centre
	Vector3d<float> simCorner(params.constants.sceneSize);
	simCorner /= 2.0f;
	simCorner += params.constants.sceneOffset;
	const float sceneDx = placementStepX * ((float)howManyAlongX-1.f)/2.f;
	const float sceneDy = placementStepY * ((float)howManyAlongY-1.f)/2.f;
	const float sceneDz = params.constants.sceneSize.z /2.f;

	fo->agentsSpatialHeatMap.reset(simCorner-Vector3d<float>(1.1f*sceneDx,1.1f*sceneDy,sceneDz),
	          simCorner+Vector3d<float>(1.1f*sceneDx,1.1f*sceneDy,sceneDz),
	          Vector3d<float>(1.8f*placementStepX,1.8f*placementStepY,params.constants.sceneSize.z) );
	fo->agentsSpatialHeatMap.printStats();

	simCorner.x -= sceneDx;
	simCorner.y -= sceneDy;

	//agents' metadata
	char agentName[512];

	const int batchSize = (int)std::ceil( howManyAlongX * howManyAlongY / P );
	REPORT("Parallel Scenario batchSize=" << batchSize);
	int createdAgents = 0;

	for (int y = 0; y < howManyAlongY; ++y)
	for (int x = 0; x < howManyAlongX; ++x)
	{
		++createdAgents; // Bug který to udělal.
		/*
		//skip this agent if it does not belong to our batch
		//REPORT("Parallel Scenario created agents/batch size = " << (createdAgents/batchSize)+1 << " p=" << p << " howManyAlongX=" << howManyAlongX << " howManyAlongY=" << howManyAlongY);
		if ((createdAgents-1)/batchSize +1 < p) continue;

		//stop creating agents if we are over with our batch
		if ((createdAgents-1)/batchSize +1 > p) break;
		*/

		//the wished position
		Vector3d<float> pos(simCorner);
		pos.x += placementStepX * (float)x;
		pos.y += placementStepY * (float)y;

		//shape (and placement)
		Spheres s(1);
		s.updateCentre(0,pos);
		s.updateRadius(0,agentRadius);

		if (createdAgents % P == (p-1))\
		{
			int ID=fo->getNextAvailAgentID();
			//name
			sprintf(agentName,"nucleus %d @ %d,%d",ID,x,y);
			ParallelNucleus* ag = new ParallelNucleus(ID,std::string(agentName),s,x,y,params.constants.initTime,params.constants.incrTime);
			fo->startNewAgent(ag);
			//DEBUG REMOVE LATER TODO
			fo->agentsSpatialHeatMap.insertAgent(*ag);
		}
	}

	/*
	fo->agentsSpatialHeatMap.stopUsing();
	//DEBUG REMOVE LATER TODO
	i3d::Image3d<i3d::GRAY8> heatImg;
	fo->agentsSpatialHeatMap.printHeatMap(heatImg);
	heatImg.SaveImage("agentsLayoutHeat.ics");

	std::list<int> agents;
	fo->agentsSpatialHeatMap.getNearbyAgentIDs( simCorner
	      +Vector3d<float>(123*placementStepX,123*placementStepY,20),
	      50,agents);
	for (int i : agents) std::cout << i << ",";
	std::cout << " (list)" << std::endl;
	*/
}
