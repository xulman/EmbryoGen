#pragma once

#include "Geometries/Geometry.hpp"
#include "Scenarios/common/Scenario.hpp"
#include "util/report.hpp"
#include "util/strings.hpp"
#include <map>
#include <vector>

class AbstractAgent;
class ShadowAgent;

class FrontOfficer {
  public:
	FrontOfficer(ScenarioUPTR s, const int myPortion, const int allPortions);

	FrontOfficer(const FrontOfficer&) = delete;
	FrontOfficer& operator=(const FrontOfficer&) = delete;

	FrontOfficer(FrontOfficer&&) = default;
	FrontOfficer& operator=(FrontOfficer&&) = default;

	~FrontOfficer();

	// ==================== Implemented in FrontOfficer.cpp ====================

	int getID() const;

	/** agent asks here for a new unique agent ID */
	int getNextAvailAgentID();

	/** stage 1/2 to do: scene heavy inits and adds agents */
	void init1();
	/** stage 2/2 to do: scene heavy inits and adds agents */
	void init2();

	/** aiders for the execute() that are called directly from it (when running
	    as distributed), or from the Direktor's execute() (when in ST mode) */
	void executeEndSub1();
	void executeEndSub2();

	/** introduces a new agent into the universe of this simulation, and,
	    optionally, it can log this event into the CTC tracking file */
	void startNewAgent(std::unique_ptr<AbstractAgent> ag,
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
	void startNewDaughterAgent(std::unique_ptr<AbstractAgent> ag,
	                           const int parentID);

	/** removes the 'mother' agent from this simulation and introduces two new
	   instead, this event is logged into the CTC tracking file automatically
	   (see the docs of startNewDaughterAgent()); for the CTC logging, it is
	   assumed that the mother is not available in the current rendered frame
	   but was (last) visible in the previous frame */
	void closeMotherStartDaughters(AbstractAgent* mother,
	                               std::unique_ptr<AbstractAgent> daughterA,
	                               std::unique_ptr<AbstractAgent> daughterB);

	/** Fills the vector 'l' of NamedAABBs that are no further than maxDist
	    parameter [micrometer]. The distance is examined as the distance
	    between AABBs (axis-aligned bounding boxes) of all agents in
	    the simulation (this->AABBs) and the reference box 'fromThisAABB'.
	    The discovered boxes are added to the output vector 'l'. The added
	    pointers points on objects contained in the vector this->AABBs.

	    Consider also the docs of translateNameIdToAgentName() to understand
	    the grand scheme of things, please. */
	void getNearbyAABBs(
	    const NamedAxisAlignedBoundingBox& fromThisAABB,    // reference box
	    const float maxDist,                                // threshold dist
	    std::vector<const NamedAxisAlignedBoundingBox*>& l) // output list
	    const;

	/** Basically, just calls
	 * getNearbyAABBs(fromSA->createNamedAABB(),maxDist,l) */
	void getNearbyAABBs(
	    const ShadowAgent* const fromSA,                    // reference agent
	    const float maxDist,                                // threshold dist
	    std::vector<const NamedAxisAlignedBoundingBox*>& l) // output list
	    const;

	/** Queries this->agentsTypesDictionary for the given 'nameID', and, if all
	   is well, returns the agent type string (ShadowAgent::agentType::_string).

	    The idea is that caller (a simulation agent) uses getNearbyAABBs() to
	   figure out who (what other simulation agents) is nearby, this comes in a
	   vector of NamedAxisAlignedBoundingBox-es. Every box, includes an ID of an
	   agent it is representing and nameID of the agent's type. The caller then
	   translates/expands the nameID into full agent's type std::string (this is
	   what this method does), decides if this nearby agent is of interest, and
	   if it is, the caller calls this->getNearbyAgent(ID from the AABB of the
	   nearby agent). */
	const std::string& translateNameIdToAgentName(const size_t nameID) const;

	/** Fills the vector 'l' of ShadowAgents that are no further than maxDist
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
	   simulation. */
	void getNearbyAgents(const ShadowAgent* const fromSA,     // reference agent
	                     const float maxDist,                 // threshold dist
	                     std::vector<const ShadowAgent*>& l); // output list

	/** Returns the reference in a ShadowAgent that
	    represents the agent with 'fetchThisID'.
	    The pointer is one from the collection of pointers in
	   this->shadowAgents.

	    Consider also the docs of translateNameIdToAgentName() to understand
	    the grand scheme of things, please. */
	const ShadowAgent* getNearbyAgent(const int fetchThisID);

	std::size_t getSizeOfAABBsList() const;

	std::vector<std::pair<int, bool>> getStartedAgents();
	std::vector<int> getClosedAgents();
	std::vector<std::pair<int, int>> getParentalLinksUpdates();

	/** notifies the agent to enable/disable its detailed drawing routines */
	void setAgentsDetailedDrawingMode(const int agentID, const bool state);

	/** notifies the agent to enable/disable its detailed reporting routines */
	void setAgentsDetailedReportingMode(const int agentID, const bool state);

	/** sets the FrontOfficer::renderingDebug flag */
	void setSimulationDebugRendering(const bool state);

	void reportSituation();
	void reportAABBs();
	void reportOverlap(const float dist);

	/** adds an item to the map this->agentsToFOsMap,
	    it should be called only from the AABB broadcast receiving methods */
	void registerThatThisAgentIsAtThisFO(const int agentID, const int FOsID);

	/** housekeeping before the AABBs exchange takes place */
	void prepareForUpdateAndPublishAgents();

	/** register the new agents, unregister the dead agents;
	    distribute the new and old existing agents to the sites */
	void updateAndPublishAgents();

	/** housekeeping after the complete AABBs exchange took place,
	    "complete" means that _all_ FOs have broadcast all they wanted */
	void postprocessAfterUpdateAndPublishAgents();

	/** the main execute() method is actually made of this one */
	void executeInternals();
	/** the main execute() method is actually made of this one */
	void executeExternals();

	/* This will be called as needed from
	 * SimulationControl/{Director, FrontOffier}*.cpp  **/
	void renderNextFrame(i3d::Image3d<i3d::GRAY16>& imgMask,
	                     i3d::Image3d<float>& imgPhantom,
	                     i3d::Image3d<float>& imgOptics);

	// ===== Implemented in SimulationControl/FrontOfficer*.cpp methods =====
	/** Initialize FO */
	void init();

	/** does the simulation loops, i.e. calls AbstractAgent's methods */
	void execute();

  private:
	// ===== Implemented in SimulationControl/FrontOfficer*.cpp methods =====
	void waitHereUntilEveryoneIsHereToo() const;
	void waitFor_publishAgentsAABBs() const;

	// this shall tell all (including this one) FOs the AABB agents,
	// the Direktor actually does not care
	void broadcast_AABBofAgents();

	std::unique_ptr<ShadowAgent> request_ShadowAgentCopy(const int agentID,
	                                                     const int FOsID) const;

	void waitFor_renderNextFrame() const;

	// ==================== private variables ====================

	ScenarioUPTR scenario;

	int ID, nextFOsID, FOsCount, nextAvailAgentID;

	float overlapMax = 0.f;
	float overlapAvg = 0.f;
	int overlapSubmissionsCounter = 0;

	/** queue of existing agents scheduled for the addition to or
	for the removal from the simulation (at the appropriate,
	occasion) and computed on this node (managed by this FO) */
	std::vector<std::unique_ptr<AbstractAgent>> newAgents;
	std::vector<int> deadAgentsIDs;

	/**
	 * Buffers filled during simulation with data, that will
	 * be later send to director (when he issues a request).
	 */
	/* std::pair<newAgentID, wantsToAppearInCTCtracksTXTfile> */
	std::vector<std::pair<int, bool>> startedAgents;

	/* std::vector<agentID> */
	std::vector<int> closedAgents;

	/* std::pair<childID, parentID> */
	std::vector<std::pair<int, int>> parentalLinks;

	/** map of all agents currently active in the simulation
	and computed on this node (managed by this FO) */
	std::map<int, std::unique_ptr<AbstractAgent>> agents;

	/** maps nameIDs (from NamedAxisAlignedBoundingBox, which should be the same
	as ShadowAgent::getAgentTypeID()) to ShadowAgent::getAgentType(),
	see, e.g., ShadowAgent::createNamedAABB() or
   FrontOfficer::broadcast_AABBofAgent() */
	StringsDictionary agentsTypesDictionary;

	/** queue of AABBs of all agents currently active in the entire
	    simulation, that is, managed by all FOs */
	std::vector<NamedAxisAlignedBoundingBox> AABBs;

	/** cache of all recently retrieved geometries of agents
	    that are computed elsewhere (managed by foreign FO) */
	std::map<int, std::unique_ptr<ShadowAgent>> shadowAgents;

	/** a complete map of all agents in the simulation (includes even earlier
	   agents) and their versions of their geometries that were broadcast the
	   most recently, this attribute works in conjunction with 'shadowAgents'
	   and getNearbyAgent() */
	std::map<int, int> agentsAndBroadcastGeomVersions;

	/** a complete map of all agents in the simulation and IDs of FOs
	    on which they are currently computed; this map is completely
	    rebuilt everytime the AABBs are exchanged */
	std::map<int, int> agentsToFOsMap;

	/** current global simulation time [min] */
	float currTime = 0.0f;

	/** counter of exports/snapshots, used to numerate frames and output image
	 * files */
	int frameCnt = 0;

	/** Flags if agents' drawForDebug() should be called with every
	 * this->renderNextFrame() */
	bool renderingDebug = false;

	/** Pointer to (yet) unknown data for implementation purposes
	 * (unique_ptr cannot point to void... => shared_ptr) **/
	std::shared_ptr<void> implementationData = nullptr;
};
