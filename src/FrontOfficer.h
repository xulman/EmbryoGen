#ifndef FRONTOFFICER_H
#define FRONTOFFICER_H

#include <list>
#include "util/report.h"
#include "Agents/AbstractAgent.h"
#include "Scenarios/common/Scenario.h"
class Director;

/** has access to Simulation, to reach its initializeAgents() */
class FrontOfficer//: public Simulation
{
public:
	FrontOfficer(Scenario& s, const int nextFO, const int myPortion, const int allPortions)
		: scenario(s), ID(myPortion), nextFOsID(nextFO), FOsCount(allPortions)
	{
		//create an extra thread to execute the respond_...() methods
	}

protected:
	Scenario& scenario;

public:
	const int ID, nextFOsID, FOsCount;

	/** good old getter for otherwise const'ed public ID */
	int getID() { return ID; }

	// ==================== simulation methods ====================
	// these are implemented in:
	// FrontOfficer.cpp

	/** scene heavy inits and adds agents */
	void init(void)
	{
		init1_SMP();
		init2_SMP();
	}

	void init1_SMP()
	{
		REPORT("FO #" << ID << " initializing now...");
		currTime = scenario.params.constants.initTime;

		scenario.initializeScene();
		scenario.initializeAgents(this,ID,FOsCount);
	}
	void init2_SMP()
	{
#ifdef DISTRIBUTED
		//in the SMP case, this method will be called explicitly
		//from the Direktor, and thus the run is synchronized here
		prepareForUpdateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		updateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();
#endif
		reportSituation();
		REPORT("FO #" << ID << " initialized");
	}

	/** does the simulation loops, i.e. calls AbstractAgent's methods */
	void execute(void);

	void reportSituation()
	{
		//overlap reports:
		if (overlapSubmissionsCounter == 0)
		{
			DEBUG_REPORT("no overlaps reported at all");
		}
		else
		{
			DEBUG_REPORT("max overlap: " << overlapMax
		        << ", avg overlap: " << (overlapSubmissionsCounter > 0 ? overlapAvg/float(overlapSubmissionsCounter) : 0.f)
		        << ", cnt of overlaps: " << overlapSubmissionsCounter);
		}
		overlapMax = overlapAvg = 0.f;
		overlapSubmissionsCounter = 0;

		REPORT("--------------- " << currTime << " min ("
		  << agents.size() << " in this FO #" << ID << " / "
		  << AABBs.size() << " AABBs (entire world), "
		  << shadowAgents.size() << " cached geometries) ---------------");
	}

	/** frees simulation agents */
	void close(void)
	{
		//mark before closing is attempted...
		isProperlyClosedFlag = true;
		DEBUG_REPORT("running the closing sequence");

		//TODO
	}

	/** attempts to clean up, if not done earlier */
	~FrontOfficer(void)
	{
		DEBUG_REPORT("FrontOfficer #" << ID << " already closed? " << (isProperlyClosedFlag ? "yes":"no"));
		if (!isProperlyClosedFlag) this->close();
	}


	/** agent asks here for a new unique agent ID */
	int getNextAvailAgentID();

	/** introduces a new agent into the universe of this simulation, and,
	    optionally, it can log this event into the CTC tracking file */
	void startNewAgent(AbstractAgent* ag, const bool wantsToAppearInCTCtracksTXTfile = true);

	/** removes the agent from this simulation, this event is logged into
	    the CTC tracking file iff the agent was registered previously;
	    for the CTC logging, it is assumed that the agent is not available in
	    the current rendered frame but was (last) visible in the previous frame */
	void closeAgent(AbstractAgent* ag);

	/** introduces a new agent into the universe of this simulation, and,
	    _automatically_, it logs this event into the CTC tracking file
	    along with the (explicit) parental information; the mother-daughter
	    relationship is purely semantical and is here only because of the
	    CTC format, that said, the simulator does not care if an agent is
	    actually a "daughter" of another agent */
	void startNewDaughterAgent(AbstractAgent* ag, const int parentID);

	/** removes the 'mother' agent from this simulation and introduces two new instead, this event
	    is logged into the CTC tracking file automatically (see the docs of startNewDaughterAgent());
	    for the CTC logging, it is assumed that the mother is not available in
	    the current rendered frame but was (last) visible in the previous frame */
	void closeMotherStartDaughters(AbstractAgent* mother,
	                               AbstractAgent* daughterA, AbstractAgent* daughterB);

	/** Fills the list 'l' of ShadowAgents that are no further than maxDist
	    parameter [micrometer]. The distance is examined as the distance
	    between AABBs (axis-aligned bounding boxes) of the ShadowAgents
	    and the given ShadowAgent. */
	void getNearbyAgents(const ShadowAgent* const fromSA,   //reference agent
	                     const float maxDist,               //threshold dist
	                     std::list<const ShadowAgent*>& l); //output list

	size_t getSizeOfAABBs()
	{ return AABBs.size(); }

	/** returns the state of the 'willRenderNextFrameFlag', that is if the
	    current simulation round with end up with the call to renderNextFrame() */
	bool willRenderNextFrame(void)
	{ return willRenderNextFrameFlag; }

	/** notifies the agent to enable/disable its detailed drawing routines */
	void setAgentsDetailedDrawingMode(const int agentID, const bool state)
	{
		//find the agentID among currently existing agents...
		for (auto ag : agents)
		if (ag->ID == agentID) ag->setDetailedDrawingMode(state);
	}

	/** notifies the agent to enable/disable its detailed reporting routines */
	void setAgentsDetailedReportingMode(const int agentID, const bool state)
	{
		//find the agentID among currently existing agents...
		for (auto ag : agents)
		if (ag->ID == agentID) ag->setDetailedReportingMode(state);
	}

protected:
	/** flag to run-once the closing routines */
	bool isProperlyClosedFlag = false;

	/** lists of existing agents scheduled for the addition to or
	    for the removal from the simulation (at the appropriate occasion) */
	std::list<AbstractAgent*> newAgents, deadAgents;

	/** list of all agents currently active in the simulation
	    and calculated on this node (managed by this FO) */
	std::list<AbstractAgent*> agents;

	/** list of all agents currently active in the simulation
	    and calculated elsewhere (managed by foreign FO) */
	std::list<ShadowAgent*> shadowAgents;

	/** list of AABBs of all agents currently active in the entire
	    simulation, that is, managed by all FOs */
	std::list<AxisAlignedBoundingBox> AABBs;

	/** current global simulation time [min] */
	float currTime = 0.0f;

	/** counter of exports/snapshots, used to numerate frames and output image files */
	int frameCnt = 0;

	/** flag if the renderNextFrame() will be called after this simulation round */
	bool willRenderNextFrameFlag = false;

	/** housekeeping before the AABBs exchange takes place */
	void prepareForUpdateAndPublishAgents();

	/** register the new agents, unregister the dead agents;
	    distribute the new and old existing agents to the sites */
	void updateAndPublishAgents();

	/** the main execute() method is actually made of this one */
	void executeInternals(void);
	/** the main execute() method is actually made of this one */
	void executeExternals(void);
	/** local counterpart to the Director::renderNextFrame() */
	void renderNextFrame(void);

	// ==================== communication methods ====================
	// these are implemented in either exactly one of the two:
	// Communication/FrontOfficerSMP.cpp
	// Communication/FrontOfficerMPI.cpp

	void waitHereUntilEveryoneIsHereToo();

	int  request_getNextAvailAgentID();

	void request_startNewAgent(const int newAgentID, const int associatedFO,
	                           const bool wantsToAppearInCTCtracksTXTfile);
	void request_closeAgent(const int agentID, const int associatedFO);
	void request_updateParentalLink(const int childID, const int parentID);

	//bool request_willRenderNextFrame();

	void waitFor_publishAgentsAABBs();
	void notify_publishAgentsAABBs(const int FOsID);
	void broadcast_AABBofAgent(const ShadowAgent& ag);
	void respond_AABBofAgent();
	void respond_CntOfAABBs();

	void respond_setDetailedDrawingMode();
	void respond_setDetailedReportingMode();

	//not revisited yet
	void respond_publishGeometry();
	void respond_renderNextFrame();

/*
naming nomenclature:
Since the methods should come in pairs, their names are made
of three elements: the action and the counter action (e.g.
ask/answer or doSmth/doneSmth), underscore, and the action
identifier that helps to see which two methods belong together.
The pair may be implemented in completely in FO, or in the Direktor,
or (not exclusively) in both. We have here the following actions:

request_* that typically blocks until the other side does or provides
          something (in which case the method has some return type);
          it is a peer to peer communication so request methods on the
          Direktor's side typically have a parameter that identifies
          particular FO, this param is typically missing on the FO side
          since there is only one Direktor (and therefore an obvious peer)

respond_* typically a counter part to the request_ method


notify_*  that is a non-blocking request until the other side does something,
          it is a "fire and forget" event, the same addressing rule applies
          as for the request methods

waitFor_* blocking counter part to the notify_ methods, the peer waits here until
          notify signal arrives, it however does not matter who has sent the signal,
          only the "type" of the signal is important as this method waits only for
          a signal of the given type


broadcast_* is similar to notify, also sends a particular signal (or data),
            it is also "fire and forget", but it addresses everyone, that is
            the Direktor and all FOs including yourself; respond_ method is
            typically the counter part to the broadcast
*/

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


#ifndef DISTRIBUTED
	Director* Direktor = NULL;

public:
	void connectWithDirektor(Director* d)
	{
		if (d == NULL) throw ERROR_REPORT("Provided Director is actually NULL.");
		Direktor = d;
	}

	//to allow the Direktor to call methods that would be otherwise
	//(in the MPI world) called from our internal methods, typically
	//from the respond_...() ones
	friend class Director;
#endif
};
#endif
