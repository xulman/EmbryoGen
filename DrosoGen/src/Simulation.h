#ifndef SIMULATION_H
#define SIMULATION_H

#include <chrono>
#include <thread>
#include <list>
#include <i3d/image3d.h>

#include "util/report.h"
#include "TrackRecord_CTC.h"
#include "util/Vector3d.h"

#include "Agents/AbstractAgent.h"

#include "DisplayUnits/VoidDisplayUnit.h"
#include "DisplayUnits/ConsoleDisplayUnit.h"
#include "DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "DisplayUnits/BroadcasterDisplayUnit.h"

//uncomment this macro to enable the system to generate images
//#define PRODUCE_IMAGES

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
 * Every agent, however, needs to examine its surrounding environment, e.g.
 * calculate distances to his neighbors, during its operation. To realize this,
 * a set of "callback" functions is provided in this class. Namely, a rough
 * distance (based on bounding boxes) and detailed (and computationally
 * expensive) distance measurements. The latter will be cached. Both
 * methods must be able to handle multi-threaded situation (should be
 * re-entrant).
 *
 * Author: Vladimir Ulman, 2018
 */
class Simulation
{
protected:
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

	/** output image into which the simulation will be iteratively rasterized/rendered: instance masks */
	i3d::Image3d<i3d::GRAY16> imgMask;

	/** output image into which the simulation will be iteratively rasterized/rendered: texture phantom image */
	i3d::Image3d<float> imgPhantom;
	//
	/** output image into which the simulation will be iteratively rasterized/rendered: optical indices image */
	i3d::Image3d<float> imgOptics;

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
	float stopTime = 200.2f;

	/** export simulation status always after this amount of global time, [min]
	    should be multiple of incrTime to obtain regular sampling */
	float expoTime = 0.5f;

	// --------------------------------------------------

	/** list of all agents currently active in the simulation
	    and calculated on this node (managed by this officer) */
	std::list<AbstractAgent*> agents;

	/** list of all agents currently active in the simulation
	    and calculated elsewhere (managed by foreign officers) */
	std::list<ShadowAgent*> shadowAgents;

	/** structure to hold durations of tracks and the mother-daughter relations */
	TrackRecords_CTC tracks;

	/** flag to run-once the closing routines */
	bool simulationProperlyClosed = false;

	// --------------------------------------------------
	// execute and maintenance methods:  init(), execute(), close()

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

		//init display/export units
		//displayUnit.RegisterUnit( new ConsoleDisplayUnit() );
		displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("localhost:8765") );
		//displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("192.168.3.110:8765") );
	}


	/** allocates output images, adds agents, renders the first frame */
	void init(void)
	{
#ifdef PRODUCE_IMAGES
		//output images that will be iteratively re-rendered
		imgMask.MakeRoom(imgSize.x,imgSize.y,imgSize.z);
		imgMask.SetResolution(i3d::Resolution(imgRes.x,imgRes.y,imgRes.z));

		imgPhantom.MakeRoom(imgSize.x,imgSize.y,imgSize.z);
		imgPhantom.SetResolution(i3d::Resolution(imgRes.x,imgRes.y,imgRes.z));

		imgOptics.MakeRoom(imgSize.x,imgSize.y,imgSize.z);
		imgOptics.SetResolution(i3d::Resolution(imgRes.x,imgRes.y,imgRes.z));
#endif

		initializeAgents();
		updateAndPublishAgents();
		REPORT("--------------- " << currTime << " min ("
		  << agents.size() << " local and "
		  << shadowAgents.size() << " shadow agents) ---------------");

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

			//develop (willingly) new shapes... (can run in parallel),
			//the agents' (external at least!) geometries must not change during this phase
			std::list<AbstractAgent*>::iterator c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->advanceAndBuildIntForces(futureTime);
#ifdef DEBUG
				if ((*c)->getLocalTime() < futureTime)
					throw new std::runtime_error("Simulation::execute(): Agent is not synchronized.");
#endif
			}

			//propagate current internal geometries to the exported ones... (can run in parallel)
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->adjustGeometryByIntForces();
				(*c)->updateGeometry();
			}

			updateAndPublishAgents();

			//react (unwillingly) to the new geometries... (can run in parallel),
			//the agents' (external at least!) geometries must not change during this phase
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->collectExtForces();
			}

			//propagate current internal geometries to the exported ones... (can run in parallel)
			c=agents.begin();
			for (; c != agents.end(); c++)
			{
				(*c)->adjustGeometryByExtForces();
				(*c)->updateGeometry();
			}

			updateAndPublishAgents();

			//overlap reports:
			DEBUG_REPORT("max overlap: " << overlapMax
			        << ", avg overlap: " << (overlapSubmissionsCounter > 0 ? overlapAvg/float(overlapSubmissionsCounter) : 0.f)
			        << ", cnt of overlaps: " << overlapSubmissionsCounter);
			overlapMax = overlapAvg = 0.f;
			overlapSubmissionsCounter = 0;

			// move to the next simulation time point
			currTime += incrTime;
			REPORT("--------------- " << currTime << " min ("
			  << agents.size() << " local and "
			  << shadowAgents.size() << " shadow agents) ---------------");

			// is this the right time to export data?
			if (currTime >= frameCnt*expoTime) renderNextFrame();
		}
	}


	/** frees simulation agents, writes the tracks.txt file */
	void close(void)
	{
		//mark before closing is attempted...
		simulationProperlyClosed = true;

		//delete all agents... also from newAgents & deadAgents, note that the
		//same agent may exist on the agents and deadAgents lists simultaneously
		std::list<AbstractAgent*>::iterator iter=agents.begin();
		while (iter != agents.end())
		{
			//CTC logging?
			if ( tracks.isTrackFollowed((*iter)->ID)      //was part of logging?
			&&  !tracks.isTrackClosed((*iter)->ID) )      //wasn't closed yet?
				tracks.closeTrack((*iter)->ID,frameCnt-1);

			//check and possibly remove from the deadAgents list
			auto daIt = deadAgents.begin();
			while (daIt != deadAgents.end() && *daIt != *iter) ++daIt;
			if (daIt != deadAgents.end())
			{
				DEBUG_REPORT("removing from deadAgents duplicate ID " << (*daIt)->ID);
				deadAgents.erase(daIt);
			}

			delete *iter; *iter = NULL;
			iter++;
		}

		tracks.exportAllToFile("tracks.txt");
		DEBUG_REPORT("tracks.txt was saved...");

		//now remove what's left on newAgents and deadAgents
		DEBUG_REPORT("will remove " << newAgents.size() << " and " << deadAgents.size()
		          << " agents from newAgents and deadAgents, respectively");
		while (!newAgents.empty())
		{
			delete newAgents.front();
			newAgents.pop_front();
		}
		while (!deadAgents.empty())
		{
			delete deadAgents.front();
			deadAgents.pop_front();
		}
	}


	/** tries to save tracks.txt at least, if not done earlier */
	virtual ~Simulation(void)
	{
		DEBUG_REPORT("simulation already closed? " << (simulationProperlyClosed ? "yes":"no"));
		if (!simulationProperlyClosed) this->close();
	}

	// --------------------------------------------------
	// execute and maintenance methods:  adding and removing agents

private:
	/** lists of existing agents scheduled for the addition to or
	    for the removal from the simulation (at the appropriate occasion) */
	std::list<AbstractAgent*> newAgents, deadAgents;

	/** register the new agents, unregister the dead agents;
	    distribute the new and old existing agents to the sites,
	    revoke/invalidate dead agents from the sites;
		 also retrieve similar requests from other co-workers */
	void updateAndPublishAgents()
	{
		//remove/unregister dead agents
		//(but keep them on the "dead list" for now)
		auto ag = deadAgents.begin();
		while (ag != deadAgents.end())
		{
			agents.remove(*ag);
			++ag;
		}

		//register the new ones (and remove from the "new born list")
		ag = newAgents.begin();
		while (ag != newAgents.end())
		{
			agents.push_back(*ag);
			ag = newAgents.erase(ag);
		}

		//now revoke/invalidate the dead ones
		ag = deadAgents.begin();
		while (ag != deadAgents.end())
		{
			//TODO... ask for the revocation
			DEBUG_REPORT("Revoke ID " << (*ag)->ID);

			delete *ag;
			ag = deadAgents.erase(ag);
		}

		//now distribute the existing ones
		ag = agents.begin();
		while (ag != agents.end())
		{
			//TODO... send the geometry update
			DEBUG_REPORT("Update ID " << (*ag)->ID);

			++ag;
		}

		//TODO... wait for and process data from co-workers
		//        and update this->shadowAgents
	}

	// --------------------------------------------------
	// service for agents (externals):  add/divide/remove agents

public:
	/** introduces a new agent into the universe of this simulation, and,
	    optionally, it can log this event into the CTC tracking file */
	void startNewAgent(AbstractAgent* ag, const bool wantsToAppearInCTCtracksTXTfile = true)
	{
		if (ag == NULL)
			throw new std::runtime_error("Simulation::startNewAgent(): refuse to include NULL agent.");

		//TODO reentrant: this method may be run multiple times in parallel
		//register the agent for adding into the system
		newAgents.push_back(ag);
		ag->setOfficer(this);

		//CTC logging?
		if (wantsToAppearInCTCtracksTXTfile) tracks.startNewTrack(ag->ID, frameCnt);
	}

	/** removes the agent from this simulation, this event is logged into
	    the CTC tracking file iff the agent was registered previously;
	    for the CTC logging, it is assumed that the agent is not available in
	    the current rendered frame but was (last) visible in the previous frame */
	void closeAgent(AbstractAgent* ag)
	{
		if (ag == NULL)
			throw new std::runtime_error("Simulation::closeAgent(): refuse to deal with NULL agent.");

		//TODO reentrant: this method may be run multiple times in parallel
		//register the agent for removing from the system
		deadAgents.push_back(ag);

		//CTC logging?
		if (tracks.isTrackFollowed(ag->ID)) tracks.closeTrack(ag->ID, frameCnt-1);
	}

	/** introduces a new agent into the universe of this simulation, and,
	    _automatically_, it logs this event into the CTC tracking file
	    along with the (explicit) parental information; the mother-daughter
	    relationship is purely semantical and is here only because of the
	    CTC format, that said, the simulator does not care if an agent is
	    actually a "daughter" of another agent */
	void startNewDaughterAgent(AbstractAgent* ag, const int parentID)
	{
		startNewAgent(ag, true);

		//CTC logging: also add the parental link
		tracks.updateParentalLink(ag->ID, parentID);
	}

	/** removes the 'mother' agent from this simulation and introduces two new instead, this event
	    is logged into the CTC tracking file automatically (see the docs of startNewDaughterAgent());
	    for the CTC logging, it is assumed that the mother is not available in
	    the current rendered frame but was (last) visible in the previous frame */
	void closeMotherStartDaughters(AbstractAgent* mother,
	                               AbstractAgent* daughterA, AbstractAgent* daughterB)
	{
		if (mother == NULL || daughterA == NULL || daughterB == NULL)
			throw new std::runtime_error("Simulation::closeMotherStartDaughters(): refuse to deal with (some) NULL agent.");

		closeAgent(mother);
		startNewDaughterAgent(daughterA, mother->ID);
		startNewDaughterAgent(daughterB, mother->ID);
		//NB: this can be extended to any number of daughters
	}

	// --------------------------------------------------
	// service for agents (externals):  getNearbyAgents()

	/** Fills the list 'l' of ShadowAgents that are no further than maxDist
	    parameter [micrometer]. The distance is examined as the distance
	    between AABBs (axis-aligned bounding boxes) of the ShadowAgents
	    and the given ShadowAgent. */
	void getNearbyAgents(const ShadowAgent* const fromSA,   //reference agent
	                     const float maxDist,               //threshold dist
	                     std::list<const ShadowAgent*>& l)  //output list
	{
		//cache fromSA's bounding box
		const AxisAlignedBoundingBox& fromAABB = fromSA->getAABB();
		const float maxDist2 = maxDist*maxDist;

		//examine all full (local) agents
		for (std::list<AbstractAgent*>::const_iterator
		     c=agents.begin(); c != agents.end(); c++)
		{
			//don't evaluate against itself
			if (*c == fromSA) continue;

			//close enough?
			if (fromAABB.minDistance((*c)->getAABB()) < maxDist2)
				l.push_back(*c);
		}

		//examine all shadow (outside) agents
		for (std::list<ShadowAgent*>::const_iterator
		     c=shadowAgents.begin(); c != shadowAgents.end(); c++)
		{
			//close enough?
			if (fromAABB.minDistance((*c)->getAABB()) < maxDist2)
				l.push_back(*c);
		}
	}

	// --------------------------------------------------
	// stats:  reportOverlap()

private:
	float overlapMax = 0.f;
	float overlapAvg = 0.f;
	int   overlapSubmissionsCounter = 0;

public:
	void reportOverlap(const float dist)
	{
		//new max overlap?
		if (dist > overlapMax) overlapMax = dist;

		//stats for calculating the average
		overlapAvg += dist;
		++overlapSubmissionsCounter;
	}

	// --------------------------------------------------
	// input & output:  initializeAgents() & renderNextFrame()

	/** setter for the argc & argv CLI params for initializeAgents() */
	void setArgs(int argc, char** argv)
	{
		this->argc = argc;
		this->argv = argv;
	}

protected:
	/** CLI params that might be considered by initializeAgents() */
	int argc;
	/** CLI params that might be considered by initializeAgents() */
	char** argv;

	/** Just initializes all agents: positions, weights, etc.,
	    and adds them into the this->agents, and into the this->tracks.
	    Additional command-line parameters may be available
	    as this->argc and this->argv. */
	virtual void initializeAgents(void) =0;

private:
	/** Flags if agents' drawForDebug() should be called with every this->renderNextFrame() */
	bool renderingDebug = false;

	/** Asks all agents to render and raster their state into this.displayUnit and the images */
	void renderNextFrame(void)
	{
		// ----------- OUTPUT EVENTS -----------
#ifdef PRODUCE_IMAGES
		//clear the output images
		imgMask.GetVoxelData()    = 0;
		imgPhantom.GetVoxelData() = 0;
		imgOptics.GetVoxelData()  = 0;
#endif

		//go over all cells, and render them
		std::list<AbstractAgent*>::const_iterator c=agents.begin();
		for (; c != agents.end(); c++)
		{
			(*c)->drawTexture(displayUnit);
			(*c)->drawMask(displayUnit);
			if (renderingDebug)
				(*c)->drawForDebug(displayUnit);

#ifdef PRODUCE_IMAGES
			(*c)->drawTexture(imgPhantom,imgOptics);
			(*c)->drawMask(imgMask);
			if (renderingDebug)
				(*c)->drawForDebug(imgMask); //TODO, should go into its own separate image
#endif
		}

		//render the current frame
		displayUnit.Flush();
		displayUnit.Tick( ("Time: "+std::to_string(currTime)).c_str() );

#ifdef PRODUCE_IMAGES
		//save the images
		static char fn[1024];
		sprintf(fn,"mask%03d.tif",frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		imgMask.SaveImage(fn);

		sprintf(fn,"phantom%03d.tif",frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		imgMask.SaveImage(fn);

		sprintf(fn,"optics%03d.tif",frameCnt);
		REPORT("Saving " << fn << ", hold on...");
		imgMask.SaveImage(fn);
#endif
		++frameCnt;

		// ----------- INPUT EVENTS -----------
		//if std::cin is closed permanently, wait here a couple of milliseconds
		//to prevent zeroMQ from flooding the Scenery
		if (std::cin.eof())
		{
			REPORT("waiting 1000 ms to give SimViewer some time to breath...")
			std::this_thread::sleep_for((std::chrono::milliseconds)1000);
			return;
		}

		//wait for key...
		char key;
		do {
			//read the key
			REPORT_NOENDL("Waiting for a key [and press Enter]: ");
			std::cin >> key;

			//memorize original key for the inspections handling
			const char ivwKey = key;

			//some known action?
			switch (key) {
			case 'Q':
				throw new std::runtime_error("Simulation::renderNextFrame(): User requested exit.");

			case 'H':
				//print summary of commands (and their keys)
				REPORT("key summary:");
				REPORT("Q - quits the program");
				REPORT("H - prints this summary");
				REPORT("E - no operation, just an empty command");
				REPORT("D - toggles whether agents' drawForDebug() is called");
				REPORT("I - toggles console (reporting) inspection of selected agents");
				REPORT("V - toggles visual inspection (in SimViewer) of selected agents");
				REPORT("W - toggles console and visual inspection of selected agents");
				break;

			case 'E':
				//empty action... just a key press eater when user gets lots in the commanding scheme
				REPORT("No action taken");
				break;

			case 'D':
				renderingDebug ^= true;
				REPORT("Debug rendering toggled to: " << (renderingDebug? "enabled" : "disabled"));
				break;

			case 'I':
			case 'V':
			case 'W':
				//inspection command(s) is followed by cell sub-command and agent ID(s)
				REPORT("Entering Inspection toggle mode: press 'e' or 'o' or '1' and agent ID to  enable its inspection");
				REPORT("Entering Inspection toggle mode: press      any key      and agent ID to disable its inspection");
				REPORT("Entering Inspection toggle mode: press  any key and 'E'     to leave the Inspection toggle mode");
				while (key != 'X')
				{
					std::cin >> key;
					bool state = (key == 'e' || key == 'o' || key == '1');

					int id;
					std::cin >> id;

					if (std::cin.good())
					{
						key = 'y'; //to detect if some agent has been modified
						for (auto c : agents)
						if (c->ID == id)
						{
							if (ivwKey != 'I') c->setDetailedDrawingMode(state);
							if (ivwKey != 'V') c->setDetailedReportingMode(state);
							REPORT((ivwKey != 'I' ? "vizu " : "")
							    << (ivwKey != 'V' ? "console " : "")
							    << "inspection" << (ivwKey == 'W'? "s " : " ")
							    << (state ? "enabled" : "disabled") << " for ID = " << id);
							key = 'Y'; //signal we got here
						}

						if (key == 'y')
							REPORT("no agent ID = " << id << " found");
					}
					else
					{
						key = 'X';          //prevent from entering this loop again (but not the outter one!)
						std::cin.clear();   //prevent from giving up reading
					}
					//NB: key is now either 'y' or 'Y', or 'X'
				}
				REPORT("Leaving Inspection toggle mode");
				break;

			default:
				//signal not-recognized action -> which means "do next simulation batch"
				key = 0;
			}
		}
		while (key != 0);
	}
};
#endif
