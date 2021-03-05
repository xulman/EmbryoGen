#ifndef DIREKTOR_H
#define DIREKTOR_H

#include <list>
#include <utility>
#include "util/report.h"
#include "TrackRecord_CTC.h"
#include "Scenarios/common/Scenario.h"

#ifdef DISTRIBUTED
#  include <thread>
#endif


class FrontOfficer;
class DistributedCommunicator;

/** has access to Scenario, to reach its doPhaseIIandIII() */
class Director
{
public:
	Director(Scenario& s, const int firstFO, const int allPortions, DistributedCommunicator * dc = NULL)
		: scenario(s), firstFOsID(firstFO), FOsCount(allPortions), communicator(dc),
#ifdef DISTRIBUTED
		  responder([this] {respond_Loop();}),
#endif
		  shallWaitForUserPromptFlag( scenario.params.shallWaitForUserPromptFlag )
	{
		scenario.declareDirektorContext();
		//TODO: create an extra thread to execute/service the respond_...() methods
	}

protected:
	Scenario& scenario;
	DistributedCommunicator * communicator;
public:
	/** the firstFOsID is where the round robin chain starts */
	const int firstFOsID, FOsCount;

	// ==================== simulation methods ====================
	// these are implemented in:
	// Director.cpp

	/** scene heavy inits and synthoscopy warm up, renders the first frame in the end;
	    similarly to FrontOfficer::initMPI(), the execution of the initialization
	    routine has to interweave with the execution of FO's init and so this
	    method is chopped into three pieces, these pieces take care of the synchronization
	    on their own when in MPI regime but during SMP the internal sync stuff does not work
	    and the sync has to be enforced from the outside */
	void initMPI(void)
	{
		init1_SMP();
		init2_SMP();
		init3_SMP();
	}

	/** stage 1/3 to do: scene heavy inits and synthoscopy warm up */
	void init1_SMP(void);
	/** stage 2/3 to do: scene heavy inits and synthoscopy warm up */
	void init2_SMP(void);
	/** stage 3/3 to do: renders the first frame */
	void init3_SMP(void);

	/** does the simulation loops, i.e. triggers calls of AbstractAgent's methods in the right order */
	void execute(void);

	/** frees simulation agents, writes the tracks.txt file */
	void close(void);

	/** attempts to clean up, if not done earlier */
	~Director(void)
	{
#ifdef DISTRIBUTED
		responder.join();
#endif
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

	/** returns the ID of FO to which a given agent is associated to */
	int getFOsIDofAgent(const int agentID);

	/** returns the state of the 'willRenderNextFrameFlag', that is if the
	    current simulation round with end up with the call to renderNextFrame() */
	bool willRenderNextFrame(void) const
	{ return willRenderNextFrameFlag; }

	/** returns the state of the 'shallWaitForUserPromptFlag', that is if an
	    user will be prompted (and the simulation would stop and wait) at
	    the end of the renderNextFrame() */
	bool willWaitForUserPrompt(void) const
	{ return shallWaitForUserPromptFlag; }

	/** sets the 'shallWaitForUserPromptFlag', making renderNextFrame() to prompt
	    the user (and stop the simulation and wait... and also become "command-able") */
	void enableWaitForUserPrompt(void)
	{ shallWaitForUserPromptFlag = true; }

	/** the opposite of the Director::enableWaitForUserPrompt() */
	void disableWaitForUserPrompt(void)
	{ shallWaitForUserPromptFlag = false; }

	/** notifies the agent to enable/disable its detailed drawing routines */
	void setAgentsDetailedDrawingMode(const int agentID, const bool state);

	/** notifies the agent to enable/disable its detailed reporting routines */
	void setAgentsDetailedReportingMode(const int agentID, const bool state);

	/** sets the Director::renderingDebug flag */
	void setSimulationDebugRendering(const bool state);

	// -------------- debug --------------
	void reportSituation();
	void reportAgentsAllocation();

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
	    maps between agent ID and FO ID associated with this agent,
	    the main purpose of this map is to know which FO to ask when
	    detailed geometry (in a form of the ShadowAgent) is needed */
	std::list< std::pair<int,int> > agents;

	/** structure to hold durations of tracks and the mother-daughter relations */
	TrackRecords_CTC tracks;

	/** current global simulation time [min] */
	float currTime = 0.0f;

	/** counter of exports/snapshots, used to numerate frames and output image files */
	int frameCnt = 0;

	/** flag if the renderNextFrame() will be called after this simulation round */
	bool willRenderNextFrameFlag = false;

	/** flag whether an user will be prompted (and the simulation would stop
	    and wait) at the end of the renderNextFrame(); this attribute actually
	    shadows/overlays over the scenario.params.shallWaitForUserPromptFlag */
	bool& shallWaitForUserPromptFlag;

	/** time period (in msecs) the simulator waits in Director::renderNextFrame()
	    when std. input is closed (after Ctrl+D) before it proceeds with the next
	    round of the simulation (and next round of rendering eventually) */
	size_t shallWaitForSimViewer_millis = 1000;

	/** Flags if agents' drawForDebug() should be called with every this->renderNextFrame() */
	bool renderingDebug = false;

	/** housekeeping before the AABBs exchange takes place */
	void prepareForUpdateAndPublishAgents();

	/** register the new agents, unregister the dead agents;
	    distribute the new and old existing agents to the sites */
	void updateAndPublishAgents();

	/** housekeeping after the complete AABBs exchange took place,
	    "complete" means that _all_ FOs have broadcast all they wanted */
	void postprocessAfterUpdateAndPublishAgents();

	/** Asks all agents to render and raster their state into displayUnit and the images */
	void renderNextFrame(void);

	// ==================== communication methods ====================
	// these are implemented in either exactly one of the two:
	// Communication/DirectorSMP.cpp
	// Communication/DirectorMPI.cpp

	void waitHereUntilEveryoneIsHereToo();

	void respond_getNextAvailAgentID();

	void respond_startNewAgent(int FO); //TBD - add parameters from start*
	void respond_closeAgent(int FO);
	void respond_updateParentalLink(int FO);

	//void respond_willRenderNextFrameFlag();

	void notify_publishAgentsAABBs(const int FOsID);
	void waitFor_publishAgentsAABBs();
	void respond_AABBofAgent();
	size_t request_CntOfAABBs(const int FOsID);

	void respond_newAgentsTypes(int noOfIncomingNewAgentTypes);

	void notify_setDetailedDrawingMode(const int FOsID, const int agentID, const bool state);
	void notify_setDetailedReportingMode(const int FOsID, const int agentID, const bool state);

	void request_renderNextFrame(const int FOsID);
	void waitFor_renderNextFrame(const int FOsID);

	void broadcast_setRenderingDebug(const bool setFlagToThis);

public:
	void broadcast_throwException(const char* exceptionMessage);
protected:
	void respond_throwException();

#ifndef DISTRIBUTED
	FrontOfficer* FO = NULL;

public:
	//provides shortcuts for the FO to the output images of
	//the Direktor to have them filled directly
	i3d::Image3d<i3d::GRAY16>& refOnDirektorsImgMask(void)
	{ return scenario.params.imgMask; }

	i3d::Image3d<float>& refOnDirektorsImgPhantom(void)
	{ return scenario.params.imgPhantom; }

	i3d::Image3d<float>& refOnDirektorsImgOptics(void)
	{ return scenario.params.imgOptics; }

	void connectWithFrontOfficer(FrontOfficer* fo)
	{
		if (fo == NULL) throw ERROR_REPORT("Provided FrontOfficer is actually NULL.");
		FO = fo;
	}
#endif
#ifdef DISTRIBUTED
	std::thread responder;
	void respond_Loop();
#endif
};
#endif
