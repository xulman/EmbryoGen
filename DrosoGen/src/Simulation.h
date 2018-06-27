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

/**
 * This class contains all simulation agents, scene and simulation
 * parameters, and takes care of the iterations of the simulation.
 *
 * Essentially, the simulation is initiated with this class's constructor,
 * iterations happen in the execute() method and the simulation is closed
 * in this class's destructor. The execute() goes over all simulation agents
 * and calls their methods (via AbstractAgent methods) in the correct order etc.
 * The execute() is essentially a "commanding" method.
 *
 * Author: Vladimir Ulman, 2018
 */
class Simulation
{
private:
	//fixed parameters of the simulation:

	//CTC drosophila:
	//x,y,z res = 2.46306,2.46306,0.492369 px/um
	//embryo along x-axis
	//bbox size around the embryo: 480,220,220 um
	//bbox size around the embryo: 1180,540,110 px

	/** set up the environment: offset of the scene [micrometer] */
	const Vector3d<float> sceneOffset;
	/** set up the environment: size of the scene [micrometer] */
	const Vector3d<float> sceneSize;

	// --------------------------------------------------

	//original res
	//const Vector3d<float>  imgRes(2.46f,2.46f,0.49f); //[pixels per micrometer]
	//
	//my res
	/** resolution of the output (phantom & mask) images [pixels per micrometer] */
	const Vector3d<float>  imgRes;

	/** size of the output (phantom & mask) images, [pixels]
	    it is a function of this->sceneSize and this->imgRes */
	const Vector3d<size_t> imgSize;

	/** output image into which the simulation will be iteratively rasterized/rendered,
	    just one image for now... */
	i3d::Image3d<i3d::GRAY16> img;

	/** output display unit into which the simulation will be iteratively rendered */
	BroadcasterDisplayUnit displayUnit;

	/** counter of exports/snapshots, used to numerate frames and output image files */
	int frameCnt = 0;

	// --------------------------------------------------

	/** current global simulation time [min] */
	float currTime = 0.0f;

	/** increment of the current global simulation time, [min]
	    represents the time step of one simulation step */
	const float incrTime = 0.1f;

	/** at what global time should the simulation stop [min] */
	const float stopTime = 1.2f;

	/** export simulation status always after this amount of global time, [min]
	    should be multiple of incrTime to obtain regular sampling */
	const float expoTime = 0.5f;

	// --------------------------------------------------

	/** list of all agents currently active in the simulation */
	std::list<AbstractAgent*> agents;

	/** structure to hold durations of tracks and the mother-daughter relations */
	TrackRecords tracks;
	bool tracksSaved = false;

	// --------------------------------------------------

public:
	/** initializes the simulation parameters */
	Simulation(void)
		: sceneOffset(0.f),             //[micrometer]
		  sceneSize(480.f,220.f,220.f), //[micrometer]
		  imgRes(2.0f,2.0f,2.0f),       //[pixels per micrometer]
		  imgSize((size_t)ceil(sceneSize.x * imgRes.x), //[pixels]
                (size_t)ceil(sceneSize.y * imgRes.y),
                (size_t)ceil(sceneSize.z * imgRes.z))
	{
		REPORT("scene size will be: "
		  << sceneSize.x << " x " << sceneSize.y << " x " << sceneSize.z
		  << " um =  "
		  << imgSize.x << " x " << imgSize.y << " x " << imgSize.z << " px");

		//init display/export units,
		//and make them persistent (to survive after this method is finished)
		//static VoidDisplayUnit vDU;
		static ConsoleDisplayUnit cDU;
		//static SceneryBufferedDisplayUnit sDU("localhost:8765");

		displayUnit.RegisterUnit(cDU);
		//displayUnit.RegisterUnit(sDU);
	}


	/** allocates output images, adds agents, renders the first frame */
	void init(void)
	{
		//output image that will be iteratively re-rendered
		img.MakeRoom(imgSize.x,imgSize.y,imgSize.z);
		img.SetResolution(i3d::Resolution(imgRes.x,imgRes.y,imgRes.z));

		//initializeAgents();
		initializeAgents_aFew();
		REPORT("--------------- " << currTime << " (" << agents.size() << " agents) ---------------");

		renderNextFrame();
	}


	/** does the simulation loops, i.e. calls AbstractAgent's methods in the right order */
	void execute(void)
	{
		//run the simulation rounds, one after another one
		while (currTime < stopTime)
		{
			//one simulation round is happening here

			//after this simulation round is done, all agents should
			//reach local times greater than this global time
			const float futureTime = currTime + incrTime -0.0001f;

			//develop (willingly) new shapes... (can run in parallel)
			std::list<AbstractAgent*>::iterator c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->advanceAndBuildIntForces(futureTime);
				(*c)->adjustGeometryByIntForces();

				#ifdef DEBUG
				if ((*c)->getLocalTime() < futureTime)
					throw new std::runtime_error("Simulation::execute(): Agent is not synchronized.");
				#endif
			}
			//propagate current internal geometries to the exported ones... (can run in parallel)
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->updateGeometry();
			}

			//react (unwillingly) to the new geometries... (can run in parallel)
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->collectExtForces();
				(*c)->adjustGeometryByExtForces();
			}

			//propagate current internal geometries to the exported ones... (can run in parallel)
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->updateGeometry();
			}

			// move to the next simulation time point
			currTime += incrTime;
			REPORT("--------------- " << currTime << " (" << agents.size() << " agents) ---------------");

			// is this the right time to export data?
			if (currTime >= frameCnt*expoTime) renderNextFrame();
		}
	}


	/** frees simulation agents, writes the tracks.txt file */
	void close(void)
	{
		//delete all agents...
		std::list<AbstractAgent*>::iterator iter=agents.begin();
		while (iter != agents.end())
		{
			tracks.closeTrack((*iter)->ID,frameCnt-1);

			delete *iter; *iter = NULL;
			iter++;
		}

		tracks.exportAllToFile("tracks.txt");
		tracksSaved = true;
		DEBUG_REPORT("tracks.txt was saved...");
	}


	/** tries to save tracks.txt at least, if not done earlier */
	~Simulation(void)
	{
		if (!tracksSaved)
		{
			tracks.exportAllToFile("tracks.txt");
			DEBUG_REPORT("tracks.txt was saved...");
		}
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
