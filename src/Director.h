#ifndef DIREKTOR_H
#define DIREKTOR_H

#include <list>
#include <utility>
#include "util/Vector3d.h"
#include "util/report.h"
#include "TrackRecord_CTC.h"
#include "Scenarios/common/Scenario.h"
class FrontOfficer;

/** has access to Simulation, to reach its doPhaseIIandIII() */
class Director
{
public:
	Director(Scenario& s, const int allPortions)
		: scenario(s), FOsCount(allPortions)
	{}

protected:
	Scenario& scenario;

public:
	const int FOsCount;

	// ==================== simulation methods ====================
	// these are implemented in:
	// Director.cpp

	/** allocates output images, renders the first frame */
	/** scene heavy inits and synthoscopy warm up */
	void init(void)
	{
		REPORT("Direktor initializing now...");

		//a bit of stats before we start...
		const auto& sSum = scenario.params.constants.sceneSize;
		Vector3d<float> sSpx(sSum);
		sSpx.elemMult(scenario.params.constants.imgRes);
		REPORT("scenario suggests that scene size will be: "
		  << sSum.x << " x " << sSum.y << " x " << sSum.z
		  << " um -> "
		  << sSpx.x << " x " << sSpx.y << " x " << sSpx.z << " px");

		scenario.initializeScene();
		scenario.initializePhaseIIandIII();

		REPORT("Direktor initialized");
	}

	/** does the simulation loops, i.e. triggers calls of AbstractAgent's methods in the right order */
	void execute(void);

	/** frees simulation agents, writes the tracks.txt file */
	void close(void)
	{
		//mark before closing is attempted...
		isProperlyClosedFlag = true;

		//TODO
	}

	/** attempts to clean up, if not done earlier */
	~Director(void)
	{
		DEBUG_REPORT("Direktor already closed? " << (isProperlyClosedFlag ? "yes":"no"));
		if (!isProperlyClosedFlag) this->close();
	}


	/** new available, not-yet-used, unique agent ID is created here */
	int getNextAvailAgentID();

	/** introduces a new agent into the universe of this simulation, and,
	    optionally, it can log this event into the CTC tracking file */
	void startNewAgent(const int agentID, const int associatedFO,
	                   const bool wantsToAppearInCTCtracksTXTfile);

	/** removes the agent from this simulation, this event is logged into
	    the CTC tracking file iff the agent was registered previously;
	    for the CTC logging, it is assumed that the agent is not available in
	    the current rendered frame but was (last) visible in the previous frame */
	void closeAgent(const int agentID, const int associatedFO);

	/** introduces a new agent into the universe of this simulation, and,
	    _automatically_, it logs this event into the CTC tracking file
	    along with the (explicit) parental information; the mother-daughter
	    relationship is purely semantical and is here only because of the
	    CTC format, that said, the simulator does not care if an agent is
	    actually a "daughter" of another agent */
	void startNewDaughterAgent(const int childID, const int parentID);

	/** returns the state of the 'willRenderNextFrameFlag', that is if the
	    current simulation round with end up with the call to renderNextFrame() */
	bool willRenderNextFrame(void)
	{ return willRenderNextFrameFlag; }

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

protected:
	/** flag to run-once the closing routines */
	bool isProperlyClosedFlag = false;

	/** last-used ID from any of the existing or former-existing agents;
	    with every new agent added, this attribute must always increase */
	int lastUsedAgentID = 0;

	/** maps of existing agents scheduled for the addition to or
	    for the removal from the simulation (at the appropriate occasion),
		 maps between agent ID and FO ID associated with this agent */
	std::list< std::pair<int,int> > newAgents, deadAgents;

	/** map of all agents currently active in the simulation,
		 maps between agent ID and FO ID associated with this agent */
	std::list< std::pair<int,int> > agents;

	/** current global simulation time [min] */
	float currTime = 0.0f;

	/** structure to hold durations of tracks and the mother-daughter relations */
	TrackRecords_CTC tracks;

	/** counter of exports/snapshots, used to numerate frames and output image files */
	int frameCnt = 0;

	/** flag if the renderNextFrame() will be called after this simulation round */
	bool willRenderNextFrameFlag = false;

	/** flag whether an user will be prompted (and the simulation would stop
	    and wait) at the end of the renderNextFrame() */
	bool shallWaitForUserPromptFlag = true;

	/** Flags if agents' drawForDebug() should be called with every this->renderNextFrame() */
	bool renderingDebug = false;

	// ==================== communication methods ====================
	// these are implemented in either exactly one of the two:
	// Communication/DirectorSMP.cpp
	// Communication/DirectorMPI.cpp

	void respond_getNextAvailAgentID();

	void respond_startNewAgent();
	void respond_closeAgent();
	void respond_updateParentalLink();

	void respond_willRenderNextFrameFlag();





	//not revisited yet
	void request_publishGeometry();
	void request_renderNextFrame();

#ifndef DISTRIBUTED
	FrontOfficer* FO = NULL;

public:
	void connectWithFrontOfficer(FrontOfficer* fo)
	{
		if (fo == NULL) ERROR_REPORT("Provided FrontOfficer is actually NULL.");
		FO = fo;
	}
#endif
};
#endif
