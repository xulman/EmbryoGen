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
	{}

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
		REPORT("FO #" << ID << " initializing now...");

		scenario.initializeScene();
		scenario.initializeAgents(this,ID,FOsCount);

		REPORT("FO #" << ID << " initialized");
	}

	/** does the simulation loops, i.e. calls AbstractAgent's methods */
	void execute(void);

	/** frees simulation agents */
	void close(void)
	{
		//mark before closing is attempted...
		isProperlyClosedFlag = true;

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

	/** returns the state of the 'willRenderNextFrameFlag', that is if the
	    current simulation round with end up with the call to renderNextFrame() */
	bool willRenderNextFrame(void);

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

	/** current global simulation time [min] */
	float currTime = 0.0f;

	// ==================== communication methods ====================
	// these are implemented in either exactly one of the two:
	// Communication/FrontOfficerSMP.cpp
	// Communication/FrontOfficerMPI.cpp

	int  request_getNextAvailAgentID();

	void request_startNewAgent(const int newAgentID, const int associatedFO,
	                           const bool wantsToAppearInCTCtracksTXTfile);
	void request_closeAgent(const int agentID, const int associatedFO);
	void request_updateParentalLink(const int childID, const int parentID);

	bool request_willRenderNextFrame();






	//not revisited yet
	void respond_publishGeometry();
	void respond_renderNextFrame();


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
		if (d == NULL) ERROR_REPORT("Provided Director is actually NULL.");
		Direktor = d;
	}
#endif
};
#endif
