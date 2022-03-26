#ifndef FRONTOFFICER_H
#define FRONTOFFICER_H

#include "Geometries/Geometry.hpp"
#include "Scenarios/common/Scenario.hpp"
#include "util/report.hpp"
#include "util/strings.hpp"
#include <list>
#include <map>

#ifdef DISTRIBUTED
#include <atomic>
#include <thread>
#endif

class AbstractAgent;
class ShadowAgent;
class Director;
class DistributedCommunicator;

/** has access to Simulation, to reach its initializeAgents() */
class FrontOfficer //: public Simulation
{
  public:
	FrontOfficer(Scenario& s,
	             const int nextFO,
	             const int myPortion,
	             const int allPortions,
	             DistributedCommunicator* dc = NULL)
	    : scenario(s), communicator(dc),
#ifdef DISTRIBUTED
	      finished(false),
#endif
	      ID(myPortion), nextFOsID(nextFO), FOsCount(allPortions),
	      __agentTypeBuf(
	          new char[StringsImprintSize]) // freed in FrontOfficer::close()
#ifdef DISTRIBUTED
	      ,
	      responder([this] { respond_Loop(); })
#endif
	{
		scenario.declareFOcontext(myPortion);
		// TODO: create an extra thread to execute/service the respond_...()
		// methods
	}

  protected:
	Scenario& scenario;
	DistributedCommunicator* communicator;
#ifdef DISTRIBUTED
	std::atomic_bool finished;
#endif

  public:
	const int ID, nextFOsID, FOsCount;

	/** good old getter for otherwise const'ed public ID */
	int getID() const { return ID; }

	// ==================== simulation methods ====================
	// these are implemented in:
	// FrontOfficer.cpp

	/** scene heavy inits and adds agents for the MPI case; in the SMP case,
	   however, the execution of this method has to interweave with the
	   execution of Director::init*() which is solved here by tearing the method
	   into (outside callable) pieces (in particular, into init1_SMP() and
	   init2_SMP()) and executed in the right order; in the MPI case, the
	   synchronization of the execution happens natively inside these methods so
	   we just call of them here */
	void initMPI(void) {
		init1_SMP();
		init2_SMP();
	}

	/** stage 1/2 to do: scene heavy inits and adds agents */
	void init1_SMP();
	/** stage 2/2 to do: scene heavy inits and adds agents */
	void init2_SMP();

	/** does the simulation loops, i.e. calls AbstractAgent's methods */
	void execute(void);

	/** aiders for the execute() that are called directly from it (when running
	    as distributed), or from the Direktor's execute() (when in SMP mode) */
	void executeEndSub1(void);
	void executeEndSub2(void);

	/** frees simulation agents */
	void close(void);

	/** attempts to clean up, if not done earlier */
	~FrontOfficer(void) {
#ifdef DISTRIBUTED
		pthread_cancel(responder.native_handle());
		responder.join();
		close_communication();
#endif
		report::debugMessage(
		    fmt::format("FrontOfficer #{} already closed? {}", ID,
		                (isProperlyClosedFlag ? "yes" : "no")));
		if (!isProperlyClosedFlag)
			this->close();
	}

	/** agent asks here for a new unique agent ID */
	int getNextAvailAgentID();

	/** introduces a new agent into the universe of this simulation, and,
	    optionally, it can log this event into the CTC tracking file */
	void startNewAgent(AbstractAgent* ag,
	                   const bool wantsToAppearInCTCtracksTXTfile = true);

	/** removes the agent from this simulation, this event is logged into
	    the CTC tracking file iff the agent was registered previously;
	    for the CTC logging, it is assumed that the agent is not available in
	    the current rendered frame but was (last) visible in the previous frame
	 */
	void closeAgent(AbstractAgent* ag);

	/** introduces a new agent into the universe of this simulation, and,
	    _automatically_, it logs this event into the CTC tracking file
	    along with the (explicit) parental information; the mother-daughter
	    relationship is purely semantical and is here only because of the
	    CTC format, that said, the simulator does not care if an agent is
	    actually a "daughter" of another agent */
	void startNewDaughterAgent(AbstractAgent* ag, const int parentID);

	/** removes the 'mother' agent from this simulation and introduces two new
	   instead, this event is logged into the CTC tracking file automatically
	   (see the docs of startNewDaughterAgent()); for the CTC logging, it is
	   assumed that the mother is not available in the current rendered frame
	   but was (last) visible in the previous frame */
	void closeMotherStartDaughters(AbstractAgent* mother,
	                               AbstractAgent* daughterA,
	                               AbstractAgent* daughterB);

	/** Fills the list 'l' of NamedAABBs that are no further than maxDist
	    parameter [micrometer]. The distance is examined as the distance
	    between AABBs (axis-aligned bounding boxes) of all agents in
	    the simulation (this->AABBs) and the reference box 'fromThisAABB'.
	    The discovered boxes are added to the output list 'l'. The added
	    pointers points on objects contained in the list this->AABBs, so
	    don't delete them after use... (AABBs manages that on its own).

	    Consider also the docs of translateNameIdToAgentName() to understand
	    the grand scheme of things, please. */
	void getNearbyAABBs(
	    const NamedAxisAlignedBoundingBox& fromThisAABB,   // reference box
	    const float maxDist,                               // threshold dist
	    std::list<const NamedAxisAlignedBoundingBox*>& l); // output list

	/** Basically, just calls
	 * getNearbyAABBs(fromSA->createNamedAABB(),maxDist,l) */
	void getNearbyAABBs(
	    const ShadowAgent* const fromSA,                   // reference agent
	    const float maxDist,                               // threshold dist
	    std::list<const NamedAxisAlignedBoundingBox*>& l); // output list

	/** Queries this->agentsTypesDictionary for the given 'nameID', and, if all
	   is well, returns the agent type string (ShadowAgent::agentType::_string).

	    The idea is that caller (a simulation agent) uses getNearbyAABBs() to
	   figure out who (what other simulation agents) is nearby, this comes in a
	   list of NamedAxisAlignedBoundingBox-es. Every box, includes an ID of an
	   agent it is representing and nameID of the agent's type. The caller then
	   translates/expands the nameID into full agent's type std::string (this is
	   what this method does), decides if this nearby agent is of interest, and
	   if it is, the caller calls this->getNearbyAgent(ID from the AABB of the
	   nearby agent). */
	const std::string& translateNameIdToAgentName(const size_t nameID);

	/** Fills the list 'l' of ShadowAgents that are no further than maxDist
	    parameter [micrometer]. The distance is examined as the distance
	    between AABBs (axis-aligned bounding boxes) among all agents in
	    the simulation and the reference agent 'fromSA'.
	    This method is essentially a shortcut getNearbyAgent() called
	   iteratively on all elements of the list obtained with getNearbyAABBs().
	    The added pointers is a subset of those from the collection
	    of pointers in this->shadowAgents.

	    Consider also the docs of translateNameIdToAgentName() to understand
	    the grand scheme of things, please.

	    This method is not optimal to use during distributed runs of the
	   simulation. Consider the DISTRIBUTED macro to see if this run is a
	   distributed one. */
	void getNearbyAgents(const ShadowAgent* const fromSA,   // reference agent
	                     const float maxDist,               // threshold dist
	                     std::list<const ShadowAgent*>& l); // output list

	/** Returns the reference in a ShadowAgent that
	    represents the agent with 'fetchThisID'.
	    The pointer is one from the collection of pointers in
	   this->shadowAgents.

	    Consider also the docs of translateNameIdToAgentName() to understand
	    the grand scheme of things, please. */
	const ShadowAgent* getNearbyAgent(const int fetchThisID);

	size_t getSizeOfAABBsList() const;

	/** returns the state of the 'willRenderNextFrameFlag', that is if the
	    current simulation round with end up with the call to renderNextFrame()
	 */
	bool willRenderNextFrame(void) const { return willRenderNextFrameFlag; }

	/** notifies the agent to enable/disable its detailed drawing routines */
	void setAgentsDetailedDrawingMode(const int agentID, const bool state);

	/** notifies the agent to enable/disable its detailed reporting routines */
	void setAgentsDetailedReportingMode(const int agentID, const bool state);

	/** sets the FrontOfficer::renderingDebug flag */
	void setSimulationDebugRendering(const bool state);

	// -------------- debug --------------
	void reportSituation();
	void reportAABBs();
	const StringsDictionary& refOnAgentsTypesDictionary() {
		return agentsTypesDictionary;
	}

  protected:
	/** flag to run-once the closing routines */
	bool isProperlyClosedFlag = false;

	/** lists of existing agents scheduled for the addition to or
	    for the removal from the simulation (at the appropriate,
	    occasion) and computed on this node (managed by this FO) */
	std::list<AbstractAgent*> newAgents, deadAgents;

	/** list of all agents currently active in the simulation
	    and computed on this node (managed by this FO) */
	std::map<int, AbstractAgent*> agents;

	/** maps nameIDs (from NamedAxisAlignedBoundingBox, which should be the same
	    as ShadowAgent::getAgentTypeID()) to ShadowAgent::getAgentType(),
	    see, e.g., ShadowAgent::createNamedAABB() or
	   FrontOfficer::broadcast_AABBofAgent() */
	StringsDictionary agentsTypesDictionary;

	/** list of AABBs of all agents currently active in the entire
	    simulation, that is, managed by all FOs */
	std::list<NamedAxisAlignedBoundingBox> AABBs;

	/** cache of all recently retrieved geometries of agents
	    that are computed elsewhere (managed by foreign FO) */
	std::map<int, ShadowAgent*> shadowAgents;

	/** a complete map of all agents in the simulation (includes even earlier
	   agents) and their versions of their geometries that were broadcast the
	   most recently, this attribute works in conjunction with 'shadowAgents'
	   and getNearbyAgent() */
	std::map<int, int> agentsAndBroadcastGeomVersions;

	/** a complete map of all agents in the simulation and IDs of FOs
	    on which they are currently computed; this map is completely
	    rebuilt everytime the AABBs are exchanged */
	std::map<int, int> agentsToFOsMap;

	/** adds an item to the map this->agentsToFOsMap,
	    it should be called only from the AABB broadcast receiving methods */
	void registerThatThisAgentIsAtThisFO(const int agentID, const int FOsID);

	/** current global simulation time [min] */
	float currTime = 0.0f;

	/** counter of exports/snapshots, used to numerate frames and output image
	 * files */
	int frameCnt = 0;

	/** flag if the renderNextFrame() will be called after this simulation round
	 */
	bool willRenderNextFrameFlag = false;

	/** Flags if agents' drawForDebug() should be called with every
	 * this->renderNextFrame() */
	bool renderingDebug = false;

	/** housekeeping before the AABBs exchange takes place */
	void prepareForUpdateAndPublishAgents();

	/** register the new agents, unregister the dead agents;
	    distribute the new and old existing agents to the sites */
	void updateAndPublishAgents();

	/** housekeeping after the complete AABBs exchange took place,
	    "complete" means that _all_ FOs have broadcast all they wanted */
	void postprocessAfterUpdateAndPublishAgents();

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

	int request_getNextAvailAgentID();

	void request_startNewAgent(const int newAgentID,
	                           const int associatedFO,
	                           const bool wantsToAppearInCTCtracksTXTfile);
	void request_closeAgent(const int agentID, const int associatedFO);
	void request_updateParentalLink(const int childID, const int parentID);

	// bool request_willRenderNextFrame();

	void waitFor_publishAgentsAABBs();
	void notify_publishAgentsAABBs(const int FOsID);
	void broadcast_AABBofAgent(const ShadowAgent& ag);
	void respond_AABBofAgent();
	void respond_CntOfAABBs();

	void broadcast_newAgentsTypes();
	void respond_newAgentsTypes(int noOfIncomingNewAgentTypes);
	char* const __agentTypeBuf; // RO pointer on RW data

	void respond_setDetailedDrawingMode();
	void respond_setDetailedReportingMode();

	ShadowAgent* request_ShadowAgentCopy(const int agentID, const int FOsID);
	void respond_ShadowAgentCopy(const int agentID);

	void waitFor_renderNextFrame(const int FOsID);
	void request_renderNextFrame(const int FOsID);

	void respond_setRenderingDebug();

#ifdef DISTRIBUTED
	void close_communication();
#endif

  public:
	void broadcast_throwException(const char* exceptionMessage);

  protected:
	void respond_throwException();

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


	notify_*  that is a non-blocking request until the other side does
	something, it is a "fire and forget" event, the same addressing rule applies
	          as for the request methods

	waitFor_* blocking counter part to the notify_ methods, the peer waits here
	until notify signal arrives, it however does not matter who has sent the
	signal, only the "type" of the signal is important as this method waits only
	for a signal of the given type


	broadcast_* is similar to notify, also sends a particular signal (or data),
	            it is also "fire and forget", but it addresses everyone, that is
	            the Direktor and all FOs including yourself; respond_ method is
	            typically the counter part to the broadcast
	*/

  private:
	float overlapMax = 0.f;
	float overlapAvg = 0.f;
	int overlapSubmissionsCounter = 0;

  public:
	void reportOverlap(const float dist) {
		// new max overlap?
		if (dist > overlapMax)
			overlapMax = dist;

		// stats for calculating the average
		overlapAvg += dist;
		++overlapSubmissionsCounter;
	}

#ifndef DISTRIBUTED
	Director* Direktor = NULL;

  public:
	void connectWithDirektor(Director* d) {
		if (d == NULL)
			throw report::rtError("Provided Director is actually NULL.");
		Direktor = d;
	}

	// to allow the Direktor to call methods that would be otherwise
	//(in the MPI world) called from our internal methods, typically
	// from the respond_...() ones
	friend class Director;
#endif
#ifdef DISTRIBUTED
	std::thread responder;
	void respond_Loop();
#endif
};
#endif
