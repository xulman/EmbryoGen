#ifndef SIMULATION_H
#define SIMULATION_H

#include <chrono>
#include <thread>
#include <list>
#include <i3d/image3d.h>

#include "util/report.h"
#include "TrackRecord_CTC.h"
#include "util/Vector3d.h"
#include "util/synthoscopy/SNR.h"

#include "Agents/AbstractAgent.h"

#include "DisplayUnits/BroadcasterDisplayUnit.h"
#include "DisplayUnits/VoidDisplayUnit.h"
#include "DisplayUnits/ConsoleDisplayUnit.h"
#include "DisplayUnits/SceneryDisplayUnit.h"
#include "DisplayUnits/SceneryBufferedDisplayUnit.h"
#include "DisplayUnits/FileDisplayUnit.h"
#include "DisplayUnits/FlightRecorderDisplayUnit.h"

//choose either ENABLE_MITOGEN_FINALPREVIEW or ENABLE_FILOGEN_PHASEIIandIII,
//if ENABLE_FILOGEN_PHASEIIandIII is choosen, one can enable ENABLE_FILOGEN_REALPSF
//#define ENABLE_MITOGEN_FINALPREVIEW
#define ENABLE_FILOGEN_PHASEIIandIII
#define ENABLE_FILOGEN_REALPSF

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

	/** output image into which the simulation will be iteratively rasterized/rendered: instance masks */
	i3d::Image3d<i3d::GRAY16> imgMask;

	/** output image into which the simulation will be iteratively rasterized/rendered: texture phantom image */
	i3d::Image3d<float> imgPhantom;
	//
	/** output image into which the simulation will be iteratively rasterized/rendered: optical indices image */
	i3d::Image3d<float> imgOptics;

	/** output image into which the simulation will be iteratively rasterized/rendered: final output image */
	i3d::Image3d<i3d::GRAY16> imgFinal;

#ifdef ENABLE_FILOGEN_REALPSF
	i3d::Image3d<float> imgPSF;
#endif

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

private:
	/** list of all agents currently active in the simulation
	    and calculated on this node (managed by this officer) */
	std::list<AbstractAgent*> agents;

	/** list of all agents currently active in the simulation
	    and calculated elsewhere (managed by foreign officers) */
	std::list<ShadowAgent*> shadowAgents;

	/** structure to hold durations of tracks and the mother-daughter relations */
	TrackRecords_CTC tracks;

	/** flag to run-once the closing routines */
	bool simulationProperlyClosedFlag = false;

	// --------------------------------------------------
	// execute and maintenance methods:  init(), execute(), close()

public:
	/** initializes the simulation parameters */
	Simulation(void)
		: sceneOffset(0.f),                     //[micrometer]
		  sceneSize(480.f,220.f,220.f)          //[micrometer]
	{
		Vector3d<float> imgRes(2.0f,2.0f,2.0f); //[pixels per micrometer]
		setOutputImgSpecs(sceneOffset,sceneSize, imgRes);

		imgRes.elemMult(sceneSize);
		REPORT("scene size will be: "
		  << sceneSize.x << " x " << sceneSize.y << " x " << sceneSize.z
		  << " um -> "
		  << imgRes.x << " x " << imgRes.y << " x " << imgRes.z << " px");

		//init display/export units
		//displayUnit.RegisterUnit( new ConsoleDisplayUnit() );
		//displayUnit.RegisterUnit( new FileDisplayUnit("debugLog.txt") );
		displayUnit.RegisterUnit( new FlightRecorderDisplayUnit("FlightRecording.txt") );
		displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("localhost:8765") );
		//displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("10.1.202.7:8765") );     //laptop @ Vlado's office
		//displayUnit.RegisterUnit( new SceneryBufferedDisplayUnit("192.168.3.110:8765") );  //PC     @ Vlado's home

#ifdef ENABLE_FILOGEN_REALPSF
		char psfFilename[] = "../2013-07-25_1_1_9_0_2_0_0_1_0_0_0_0_9_12.ics";
		REPORT("reading this PSF image " << psfFilename);
		imgPSF.ReadImage(psfFilename);
#endif
	}

private:
	/** internal (private) memory of the input of setOutputImgSpecs() for the enableProducingOutput() */
	Vector3d<size_t> lastUsedImgSize;
public:
	/** util method to setup all output images at once, disables all of them for the output,
	    and hence does not allocate memory for the images */
	void setOutputImgSpecs(const Vector3d<float>& imgOffsetInMicrons,
	                       const Vector3d<float>& imgSizeInMicrons)
	{
		setOutputImgSpecs(imgOffsetInMicrons,imgSizeInMicrons,
		                  Vector3d<float>(imgMask.GetResolution().GetRes()));
	}

	/** util method to setup all output images at once, disables all of them for the output,
	    and hence does not allocate memory for the images */
	void setOutputImgSpecs(const Vector3d<float>& imgOffsetInMicrons,
	                       const Vector3d<float>& imgSizeInMicrons,
	                       const Vector3d<float>& imgResolutionInPixelsPerMicron)
	{
		//sanity checks:
		if (!imgSizeInMicrons.elemIsGreaterThan(Vector3d<float>(0)))
			throw ERROR_REPORT("image dimensions (size) cannot be zero or negative along any axis");
		if (!imgResolutionInPixelsPerMicron.elemIsGreaterThan(Vector3d<float>(0)))
			throw ERROR_REPORT("image resolution cannot be zero or negative along any axis");

		//metadata...
		imgMask.SetResolution(i3d::Resolution( imgResolutionInPixelsPerMicron.toI3dVector3d() ));
		imgMask.SetOffset( imgOffsetInMicrons.toI3dVector3d() );

		//disable usage of this image (for now)
		disableProducingOutput(imgMask);

		//but remember what the correct pixel size would be
		lastUsedImgSize.from(
		  Vector3d<float>(imgSizeInMicrons).elemMult(imgResolutionInPixelsPerMicron).elemCeil() );

		//propagate also the same setting onto the remaining images
		imgPhantom.CopyMetaData(imgMask);
		imgOptics.CopyMetaData(imgMask);
		imgFinal.CopyMetaData(imgMask);
	}

	/** util method to enable the given image for the output, the method
	    immediately allocated the necessary memory for the image */
	template <typename T>
	void enableProducingOutput(i3d::Image3d<T>& img)
	{
		if ((long)&img == (long)&imgFinal && !isProducingOutput(imgPhantom))
			REPORT("WARNING: Requested synthoscopy but phantoms may not be produced.");

		DEBUG_REPORT("allocating "
		  << (lastUsedImgSize.x*lastUsedImgSize.y*lastUsedImgSize.z/(1 << 20))*sizeof(*img.GetFirstVoxelAddr())
		  << " MB of memory for image of size " << lastUsedImgSize << " px");
		img.MakeRoom( lastUsedImgSize.toI3dVector3d() );
	}

	/** util method to disable the given image for the output, the method
	    immediately frees the allocated memory of the image */
	template <typename T>
	void disableProducingOutput(i3d::Image3d<T>& img)
	{
		//image size flags if this image should be iteratively filled and saved:
		//zero image size signals "don't use this image"
		img.MakeRoom(0,0,0);
	}

	/** util method to report if the given image is enabled for the output */
	template <typename T>
	bool isProducingOutput(const i3d::Image3d<T>& img) const
	{
		return (img.GetImageSize() > 0);
	}


	/** allocates output images, adds agents, renders the first frame */
	void init(void);

	/** does the simulation loops, i.e. calls AbstractAgent's methods in the right order */
	void execute(void);

	/** frees simulation agents, writes the tracks.txt file */
	void close(void);

	/** tries to save tracks.txt at least, if not done earlier */
	virtual ~Simulation(void)
	{
		DEBUG_REPORT("simulation already closed? " << (simulationProperlyClosedFlag ? "yes":"no"));
		if (!simulationProperlyClosedFlag) this->close();
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
			//DEBUG_REPORT("Revoke ID " << (*ag)->ID);

			delete *ag;
			ag = deadAgents.erase(ag);
		}

		//now distribute the existing ones
		ag = agents.begin();
		while (ag != agents.end())
		{
			//TODO... send the geometry update
			//DEBUG_REPORT("Update ID " << (*ag)->ID);

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
			throw ERROR_REPORT("refuse to include NULL agent.");

		//TODO reentrant: this method may be run multiple times in parallel
		//register the agent for adding into the system
		newAgents.push_back(ag);
		ag->setOfficer(this);

		//CTC logging?
		if (wantsToAppearInCTCtracksTXTfile) tracks.startNewTrack(ag->ID, frameCnt);

		DEBUG_REPORT("just registered this new agent: " << ag->ID << "-" << ag->getAgentType());
	}

	/** removes the agent from this simulation, this event is logged into
	    the CTC tracking file iff the agent was registered previously;
	    for the CTC logging, it is assumed that the agent is not available in
	    the current rendered frame but was (last) visible in the previous frame */
	void closeAgent(AbstractAgent* ag)
	{
		if (ag == NULL)
			throw ERROR_REPORT("refuse to deal with NULL agent.");

		//TODO reentrant: this method may be run multiple times in parallel
		//register the agent for removing from the system
		deadAgents.push_back(ag);

		//CTC logging?
		if (tracks.isTrackFollowed(ag->ID)) tracks.closeTrack(ag->ID, frameCnt-1);

		DEBUG_REPORT("just unregistered this dead agent: " << ag->ID << "-" << ag->getAgentType());
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
			throw ERROR_REPORT("refuse to deal with (some) NULL agent.");

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
	                     std::list<const ShadowAgent*>& l); //output list

	/** returns the state of the 'willRenderNextFrameFlag', that is if the
	    current simulation round with end up with the call to renderNextFrame() */
	bool willRenderNextFrame(void)
	{ return willRenderNextFrameFlag; }

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
	// input & output:  initializeScenario() & renderNextFrame()

	/** setter for the argc & argv CLI params for initializeScenario() */
	void setArgs(int argc, char** argv)
	{
		this->argc = argc;
		this->argv = argv;
	}

protected:
	/** CLI params that might be considered by initializeScenario() */
	int argc;
	/** CLI params that might be considered by initializeScenario() */
	char** argv;

	/** Just initializes all agents: positions, weights, etc.,
	    and adds them into the this->agents, and into the this->tracks.
	    Additional command-line parameters may be available
	    as this->argc and this->argv. */
	virtual void initializeScenario(void) =0;

	/** returns the state of the 'shallWaitForUserPromptFlag', that is if an
	    user will be prompted (and the simulation would stop and wait) at
	    the end of the renderNextFrame() */
	bool willWaitForUserPrompt(void)
	{ return shallWaitForUserPromptFlag; }

	/** sets the 'shallWaitForUserPromptFlag', making renderNextFrame() to prompt
	    the user (and stop the simulation and wait... and also become "command-able") */
	void enableWaitForUserPrompt(void)
	{ shallWaitForUserPromptFlag = true; }

	/** the opposite of the Simulation::enableWaitForUserPrompt() */
	void disableWaitForUserPrompt(void)
	{ shallWaitForUserPromptFlag = false; }

private:
	/** flag if the renderNextFrame() will be called after this simulation round */
	bool willRenderNextFrameFlag = false;

	/** flag whether an user will be prompted (and the simulation would stop
	    and wait) at the end of the renderNextFrame() */
	bool shallWaitForUserPromptFlag = true;

	/** Flags if agents' drawForDebug() should be called with every this->renderNextFrame() */
	bool renderingDebug = false;

	/** Asks all agents to render and raster their state into this.displayUnit and the images */
	void renderNextFrame(void);

protected:
	/** Initializes (allocates, sets resolution, etc.) and populates (fills content)
	    the Simulation::imgFinal, which will be saved as the final testing image,
	    based on the current content of the phantom and/or optics and/or mask image.

	    Depending on the scenario used, some of these (phantom, optics, mask) images
	    might be empty (voxels are zero), or their image size may be actually be zero.
	    This really depends on what agents are used and how they are designed. Which
	    is why this method has became "virtual" and over-ridable in every scenario
	    to suit its needs.

	    The content of the Simulation::imgPhantom and/or imgOptics images can be
	    altered in this method because the said variables shall not be used anymore
	    in the (just finishing) simulation round. Don't change Simulation::imgMask
	    because this one is used for computation of the SNR. */
	virtual void doPhaseIIandIII(void);
};
#endif
