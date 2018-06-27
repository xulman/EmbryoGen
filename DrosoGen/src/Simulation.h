#ifndef SIMULATION_H
#define SIMULATION_H

#include <list>
#include <i3d/image3d.h>

#include "util/report.h"
#include "TrackRecord.h"
#include "util/Vector3d.h"

#include "Agents/AbstractAgent.h"

#include "DisplayUnits/VoidDisplayUnit.h"
#include "DisplayUnits/ConsoleDisplayUnit.h"
#include "DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "DisplayUnits/BroadcasterDisplayUnit.h"

/** This class contains all simulation agents, scene and simulation
    parameters, and takes care of the iterations of the simulation */
class Simulation
{
private:
	//fixed parameters of the simulation:

	//CTC drosophila:
	//x,y,z res = 2.46306,2.46306,0.492369 px/um
	//embryo along x-axis
	//bbox size around the embryo: 480,220,220 um
	//bbox size around the embryo: 1180,540,110 px

	//set up the environment
	const Vector3d<float> sceneOffset; //[micrometer]
	const Vector3d<float> sceneSize;   //[micrometer]

	// --------------------------------------------------

	//original res
	//const Vector3d<float>  imgRes(2.46f,2.46f,0.49f); //[pixels per micrometer]
	//
	//my res
	const Vector3d<float>  imgRes;  //[pixels per micrometer]
	const Vector3d<size_t> imgSize; //[pixels]

	/*
	imgMaskFilename="Mask%05d.tif";
	imgPhantomFilename="Phantom%05d.tif";
	imgFluoFilename="FluoImg%05d.tif";
	tracksFilename="tracks.txt";
	*/

	//output image that will be iteratively re-rendered
	i3d::Image3d<i3d::GRAY16> img;

	//output display unit
	BroadcasterDisplayUnit displayUnit;

	//export counter
	int frameCnt = 0;

	// --------------------------------------------------

	/// current global simulation time [min]
	float currTime = 0.0f;

	/// increment of the current global simulation time, [min]
	/// represents the time step of one simulatino step
	const float incrTime = 0.1f;

	/// at what global time should the simulation stop [min]
	const float stopTime = 1.2f;

	/// export simulation status always after this amount of global time, [min]
	/// should be multiple of incrTime
	const float expoTime = 0.5f;

	// --------------------------------------------------

	//the list of all agents
	std::list<AbstractAgent*> agents;

	//list of agents to be removed
	std::list<AbstractAgent*> dead_agents;

	//the structure to hold all track data
	TrackRecords tracks;

	// --------------------------------------------------

public:
	/** initializes the simulation parameters, adds agents, renders the first frame */
	Simulation(void)
		: sceneOffset(0.f),             //[micrometer]
		  sceneSize(480.f,220.f,220.f), //[micrometer]
		  imgRes(2.0f,2.0f,2.0f),       //[pixels per micrometer]
		  imgSize((size_t)ceil(sceneSize.x * imgRes.x), //[pixels]
                (size_t)ceil(sceneSize.y * imgRes.y),
                (size_t)ceil(sceneSize.z * imgRes.z))
	{
		REPORT("scene size: "
		  << sceneSize.x << " x " << sceneSize.y << " x " << sceneSize.z
		  << " um =  "
		  << imgSize.x << " x " << imgSize.y << " x " << imgSize.z << " px");

		//output image that will be iteratively re-rendered
		img.MakeRoom(imgSize.x,imgSize.y,imgSize.z);
		img.SetResolution(i3d::Resolution(imgRes.x,imgRes.y,imgRes.z));

		//init display/export units,
		//and make them persistent (to survive after this method is finished)
		//static VoidDisplayUnit vDU;
		static ConsoleDisplayUnit cDU;
		//static SceneryBufferedDisplayUnit sDU("localhost:8765");

		displayUnit.RegisterUnit(cDU);
		//displayUnit.RegisterUnit(sDU);

		// --------------------------------------------------

		//initializeAgents();
		initializeAgents_aFew();
		REPORT("--init--------- " << currTime << " (" << agents.size() << " agents) ---------------");

		renderNextFrame();
	}


	/** does the simulation loops, i.e. calls AbstractAgent's methods in the right order */
	void execute(void)
	{
		//run the simulation
		while (currTime < stopTime)
		{
			//obtain develop new shapes... (can run in parallel)
			std::list<AbstractAgent*>::iterator c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->advanceAndBuildIntForces();
				(*c)->adjustGeometryByIntForces();
				(*c)->updateGeometry();
			}

			//react to the new geometries ... (can run in parallel)
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->collectExtForces();
				(*c)->adjustGeometryByExtForces();
				(*c)->updateGeometry();
			}

			// -------------------------------------

			//remove removable (can't run in parallel)
			c=dead_agents.begin();
			while (c != dead_agents.end())
			{
				delete (*c);
				c=dead_agents.erase(c);
			}

			//TODO: remove after sometime
			if (dead_agents.size() != 0)
				REPORT("CONFUSION: dead_agents IS NOT EMPTY!");

			/*
			//create removable (can't run in parallel)
			c=agents.begin();
			while (c != agents.end())
			{
				if ((*c)->shouldDie)
				{
					//schedule this one for removal
					dead_agents.push_back(*c);

					if (tracks[(*c)->ID].toTimeStamp > -1)
						REPORT("CONFUSION: dying cell HAS CLOSED TRACK!");
					tracks[(*c)->ID].toTimeStamp=frameCnt-1;

					c=agents.erase(c);
					//this removed the agent from the list of active cells,
					//but does not remove it from the memory!
					//(so that other agents that still hold _pointer_
					//on this one in their Cell::listOfFriends lists
					//can _safely_ notice this one has Cell::shouldDie
					//flag on and take appropriate action)
					//
					//after the next round of Cell::applyForces() is over,
					//all agents have definitely removed this agent from
					//their Cell::listOfFriends, that is when we can delete
					//this agent from the memory
				}
				else c++;
			}
			*/
	#ifdef DEBUG
			if (dead_agents.size() > 0)
				REPORT(dead_agents.size() << " cells will be removed from memory");
	#endif

			// -------------------------------------

			// move to the next simulation time point
			currTime += incrTime;
			REPORT("--------------- " << currTime << " (" << agents.size() << " agents) ---------------");

			// is this the right time to export data?
			if (currTime >= frameCnt*expoTime) renderNextFrame();
		}
	}


	/** the destructor frees simulation agents, writes the tracks.txt file */
	~Simulation()
	{
		//delete all agents...
		std::list<AbstractAgent*>::iterator iter=agents.begin();
		while (iter != agents.end())
		{
			tracks.closeTrack((*iter)->ID,frameCnt-1);

			delete *iter; *iter = NULL;
			iter++;
		}
		DEBUG_REPORT("all agents were removed...");

		tracks.exportAllToFile("tracks.txt");
		DEBUG_REPORT("tracks.txt was saved...");
	}


private:
	/** Just initializes all agents: positions, weights, etc.,
	    and adds them into the this->agents, and into the this->tracks. */
	void initializeAgents(void);
	void initializeAgents_aFew(void);

	/** Asks all agents to render and raster their state into this.displayUnit and this.img */
	void renderNextFrame(void)
	{
		static char fn[1024];

		//clear the output image
		//img.GetVoxelData() = 0;

		//go over all cells, and render them
		std::list<AbstractAgent*>::const_iterator c=agents.begin();
		for (; c != agents.end(); c++)
		{
			(*c)->drawMask(displayUnit);
			(*c)->drawMask(img);
		}

		//render the current frame
		displayUnit.Flush();

		//save the image
		sprintf(fn,"mask%03d.tif",frameCnt++);
		//img.SaveImage(fn);

		//wait for key...
		REPORT_NOENDL("Wait for key [and press Enter]: ");
		std::cin >> fn[0];
	}
};
#endif
