#include "FrontOfficer.hpp"
#include "Agents/AbstractAgent.hpp"
#include "Director.hpp"

void FrontOfficer::init1() {
	report::message(fmt::format("FO #{} initializing now...", ID));
	currTime = scenario->params->constants.initTime;

	scenario->initializeScene();
	scenario->initializeAgents(this, ID, FOsCount);
}

void FrontOfficer::init2() {
	reportSituation();
	report::message(fmt::format("FO #{} initialized", ID));
}

void FrontOfficer::reportSituation() {
	// overlap reports:
	if (overlapSubmissionsCounter == 0) {
		report::debugMessage(fmt::format("no overlaps reported at all"));
	} else {
		report::debugMessage(fmt::format(
		    "max overlap: {}, avg overlap: {}, cnt of overlaps: {}", overlapMax,
		    (overlapSubmissionsCounter > 0
		         ? overlapAvg / float(overlapSubmissionsCounter)
		         : 0.f),
		    overlapSubmissionsCounter));
	}
	overlapMax = overlapAvg = 0.f;
	overlapSubmissionsCounter = 0;

	report::message(fmt::format(
	    "--------------- {} min ({} in this FO #{} / {} AABBs (entire world), "
	    "{} cached geometries) ---------------",
	    currTime, agents.size(), ID, AABBs.size(), shadowAgents.size()));
}

FrontOfficer::~FrontOfficer() {
	report::debugMessage(fmt::format("running the closing sequence"));

	// TODO: should close/kill the service thread too

	// delete all agents... also later from newAgents & deadAgents, note that
	// the same agent may exist on the agents and deadAgents lists
	// simultaneously
	report::debugMessage(
	    fmt::format("will remove {} active agents", agents.size()));

	// also clean up any shadow agents one may have
	report::debugMessage(
	    fmt::format("will remove {} shadow agents", shadowAgents.size()));
}

void FrontOfficer::executeEndSub1() {
	// move to the next simulation time point
	currTime += scenario->params->constants.incrTime;
	reportSituation();
}

void FrontOfficer::executeEndSub2() {
	// this was promised to happen after every simulation round is over
	scenario->updateScene(currTime);
}

void FrontOfficer::executeInternals() {
	// after this simulation round is done, all agents should
	// reach local times greater than this global time
	const float futureTime =
	    currTime + scenario->params->constants.incrTime - 0.0001f;

	// develop (willingly) new shapes... (can run in parallel),
	// the agents' (external at least!) geometries must not change during this
	// phase
	for (auto& [_, ag] : agents) {
		ag->advanceAndBuildIntForces(futureTime);
#ifndef NDEBUG
		if (ag->getLocalTime() < futureTime)
			throw report::rtError("Agent is not synchronized.");
#endif
	}

	// propagate current internal geometries to the exported ones... (can run in
	// parallel)
	for (auto& [_, ag] : agents) {
		ag->adjustGeometryByIntForces();
		ag->publishGeometry();
	}
}

void FrontOfficer::executeExternals() {
#ifndef NDEBUG
	reportAABBs();
#endif
	// react (unwillingly) to the new geometries... (can run in parallel),
	// the agents' (external at least!) geometries must not change during this
	// phase
	for (auto& [_, ag] : agents)
		ag->collectExtForces();

	// propagate current internal geometries to the exported ones... (can run in
	// parallel)
	for (auto& [_, ag] : agents) {
		ag->adjustGeometryByExtForces();
		ag->publishGeometry();
	}
}

void FrontOfficer::prepareForUpdateAndPublishAgents() {
	AABBs.clear();
	agentsToFOsMap.clear();
	// the agentsAndBroadcastGeomVersions is cummulative, we never erase it
}

void FrontOfficer::updateAndPublishAgents() {
	// since the last call of this method, we have:
	// list of agents that were active after the last call in 'agents'
	// subset of these that became inactive in 'deadAgents'
	// list of newly created in 'newAgents'

	// we want only to put our 'agents' list into order:
	// remove/unregister dead agents
	for (auto id : deadAgentsIDs)
		agents.erase(id);

	deadAgentsIDs.clear();

	// register the new ones (and remove from the "new born list")
	for (auto& ag_ptr : newAgents) {
#ifndef NDEBUG
		if (agents.contains(ag_ptr->ID))
			throw report::rtError(fmt::format(
			    "Attempting to add another agent with the same ID {}",
			    ag_ptr->ID));
#endif
		agents[ag_ptr->ID] = std::move(ag_ptr);
	}
	newAgents.clear();

	// WAIT HERE UNTIL WE'RE TOLD TO START BROADCASTING OUR CHANGES
	// only FO with a "token" does broadcasting, token passing
	// is the same as for rendering/building output images (the round robin)
	waitFor_publishAgentsAABBs();

	// now distribute AABBs of the existing ones
	broadcast_AABBofAgents();

	// and we move on to get blocked on any following checkpoint
	// while waiting there, our respond_AABBofAgent() collects data
}

void FrontOfficer::postprocessAfterUpdateAndPublishAgents() {
	// post-process local Dictionary
	agentsTypesDictionary.markAllWasBroadcast();
	agentsTypesDictionary.cleanUp(AABBs);
}

int FrontOfficer::getNextAvailAgentID() { return nextAvailAgentID++; }

void FrontOfficer::startNewAgent(std::unique_ptr<AbstractAgent> ag,
                                 const bool wantsToAppearInCTCtracksTXTfile) {
	if (ag == nullptr)
		throw report::rtError("refuse to include NULL agent.");

	// register the agent for adding into the system:
	// local registration:
	auto ag_ptr = ag.get();
	newAgents.push_back(std::move(ag));
	agentsTypesDictionary.registerThisString(
	    ag_ptr->getAgentType_hashedString());
	ag_ptr->setOfficer(this);

	// remote registration:
	startedAgents.emplace_back(ag_ptr->ID, wantsToAppearInCTCtracksTXTfile);

	report::debugMessage(fmt::format("just registered this new agent: {}",
	                                 ag_ptr->getSignature()));
}

void FrontOfficer::closeAgent(AbstractAgent* ag) {
	if (ag == nullptr)
		throw report::rtError(fmt::format("refuse to deal with NULL agent."));

	// register the agent for removing from the system:
	// local registration
	deadAgentsIDs.push_back(ag->ID);

	// remote registration
	closedAgents.emplace_back(ag->ID);

	report::debugMessage(fmt::format("just unregistered this dead agent: {}",
	                                 ag->getSignature()));
}

void FrontOfficer::startNewDaughterAgent(std::unique_ptr<AbstractAgent> ag,
                                         const int parentID) {
	// CTC logging: also add the parental link
	parentalLinks.emplace_back(ag->ID, parentID);
	startNewAgent(std::move(ag), true);
}

void FrontOfficer::closeMotherStartDaughters(
    AbstractAgent* mother,
    std::unique_ptr<AbstractAgent> daughterA,
    std::unique_ptr<AbstractAgent> daughterB) {
	if (mother == nullptr || daughterA == nullptr || daughterB == nullptr)
		throw report::rtError("refuse to deal with (some) NULL agent.");

	closeAgent(mother);
	startNewDaughterAgent(std::move(daughterA), mother->ID);
	startNewDaughterAgent(std::move(daughterB), mother->ID);
	// NB: this can be extended to any number of daughters
}

void FrontOfficer::setAgentsDetailedDrawingMode(const int agentID,
                                                const bool state) {
	// find the agentID among currently existing agents...
	auto it = agents.find(agentID);
#ifndef NDEBUG
	if (it == agents.end())
		throw report::rtError(fmt::format("Cannot set detailed drawing mode of "
		                                  "agent: {} ... agent does not exist",
		                                  agentID));
#endif
	it->second->setDetailedDrawingMode(state);
}

void FrontOfficer::setAgentsDetailedReportingMode(const int agentID,
                                                  const bool state) {
	// find the agentID among currently existing agents...
	auto it = agents.find(agentID);
#ifndef NDEBUG
	if (it == agents.end())
		throw report::rtError(
		    fmt::format("Cannot set detailed reporting mode of "
		                "agent: {} ... agent does not exist",
		                agentID));
#endif
	it->second->setDetailedReportingMode(state);
}

void FrontOfficer::setSimulationDebugRendering(const bool state) {
	renderingDebug = state;
}

size_t FrontOfficer::getSizeOfAABBsList() const { return AABBs.size(); }

void FrontOfficer::registerThatThisAgentIsAtThisFO(const int agentID,
                                                   const int FOsID) {
#ifndef NDEBUG
	if (agentsToFOsMap.find(agentID) != agentsToFOsMap.end())
		throw report::rtError(
		    fmt::format("Agent ID {} already registered with FO #{} but was "
		                "(again) broadcast by FO #{}",
		                agentID, agentsToFOsMap.find(agentID)->second, FOsID));
#endif
	agentsToFOsMap[agentID] = FOsID;
}

void FrontOfficer::getNearbyAABBs(
    const ShadowAgent* const fromSA,                   // reference agent
    const float maxDist,                               // threshold dist
    std::deque<const NamedAxisAlignedBoundingBox*>& l) // output list
{
	getNearbyAABBs(NamedAxisAlignedBoundingBox(fromSA->getAABB(),
	                                           fromSA->getID(),
	                                           fromSA->getAgentTypeID()),
	               maxDist, l);
}

void FrontOfficer::getNearbyAABBs(
    const NamedAxisAlignedBoundingBox& fromThisAABB,   // reference box
    const float maxDist,                               // threshold dist
    std::deque<const NamedAxisAlignedBoundingBox*>& l) // output list
{
	const float maxDist2 = maxDist * maxDist;

	// examine all available boxes/agents
	for (const auto& b : AABBs) {
		// don't evaluate against itself
		if (b.ID == fromThisAABB.ID)
			continue;

		// close enough?
		if (fromThisAABB.minDistance(b) < maxDist2)
			l.push_back(&b);
	}
}

const std::string&
FrontOfficer::translateNameIdToAgentName(const size_t nameID) {
	return agentsTypesDictionary.translateIdToString(nameID);
}

void FrontOfficer::getNearbyAgents(
    const ShadowAgent* const fromSA,   // reference agent
    const float maxDist,               // threshold dist
    std::deque<const ShadowAgent*>& l) // output list
{
	// list with pointers on (nearby) boxes that live in this->AABBs,
	// these were not created explicitly and so we must not delete them
	std::deque<const NamedAxisAlignedBoundingBox*> nearbyBoxes;

	getNearbyAABBs(NamedAxisAlignedBoundingBox(fromSA->getAABB(),
	                                           fromSA->getID(),
	                                           fromSA->getAgentTypeID()),
	               maxDist, nearbyBoxes);

	for (auto box : nearbyBoxes)
		l.push_back(getNearbyAgent(box->ID));
}

const ShadowAgent* FrontOfficer::getNearbyAgent(const int fetchThisID) {
	// is the requested agent living in the same (*this) FO?
	const auto ag = agents.find(fetchThisID);
	if (ag != agents.end())
		return ag->second.get();

		// no, the requested agent is somewhere outside...
#ifndef NDEBUG
	// btw: must have been broadcasted and we must therefore see the agent in
	// our data structures
	if (agentsToFOsMap.find(fetchThisID) == agentsToFOsMap.end())
		throw report::rtError(
		    fmt::format("Should not happen! I don't know which FO is the "
		                "requested agent ID {}",
		                fetchThisID));
	if (agentsAndBroadcastGeomVersions.find(fetchThisID) ==
	    agentsAndBroadcastGeomVersions.end())
		throw report::rtError(
		    fmt::format("Should not happen! I don't know what is the recent "
		                "geometry version of the requested agent ID {}",
		                fetchThisID));
#endif

	// now, check we have a copy at all and if it is an updated/fresh one
	const auto saItem = shadowAgents.find(fetchThisID);
	int storedVersion = (saItem != shadowAgents.end())
	                        ? saItem->second->getGeometry().version
	                        : -1000000;

	// if we have a recent geometry by us, let's just return this one
	if (storedVersion == agentsAndBroadcastGeomVersions[fetchThisID])
		return saItem->second.get();

	// else, we have to obtain the most recent copy...
	const int contactThisFO = agentsToFOsMap[fetchThisID];
	report::debugMessage(fmt::format("Requesting agent ID {} from FO #{}",
	                                 fetchThisID, contactThisFO));

	// store the new reference
	shadowAgents[fetchThisID] =
	    request_ShadowAgentCopy(fetchThisID, contactThisFO);

#ifndef NDEBUG
	// now the broadcast version must match the one we actually have got
	if (agentsAndBroadcastGeomVersions[fetchThisID] !=
	    shadowAgents[fetchThisID]->getGeometry().version)
		throw report::rtError(fmt::format(
		    "Should not happen! Agent ID {} promised geometry at version {} "
		    "but provided version {}",
		    fetchThisID, agentsAndBroadcastGeomVersions[fetchThisID],
		    shadowAgents[fetchThisID]->getGeometry().version));
#endif

	return shadowAgents[fetchThisID].get();
}

void FrontOfficer::renderNextFrame(i3d::Image3d<i3d::GRAY16>& imgMask,
                                   i3d::Image3d<float>& imgPhantom,
                                   i3d::Image3d<float>& imgOptics) {
	report::message(fmt::format("Rendering time point {}", frameCnt));
	SceneControls& sc = *scenario->params;

	// ----------- OUTPUT EVENTS -----------

	// go over all cells, and render them -- ONLY IMAGES!
	for (auto& [_, ag] : agents) {
		// raster images may not necessarily always exist,
		// always check for their availability first:
		if (sc.isProducingOutput(imgPhantom) &&
		    sc.isProducingOutput(imgOptics)) {
			ag->drawTexture(imgPhantom, imgOptics);
		}
		if (sc.isProducingOutput(imgMask)) {

			ag->drawMask(imgMask);
			if (renderingDebug)
				ag->drawForDebug(imgMask);
			// TODO, should go into its
			// own separate image
		}
	}
	// note that this far the code was executed on all FOs, that means in
	// parallel

	// go over all cells, and render them -- ONLY DISPLAY UNITS!
	for (auto& [_, ag] : agents) {
		ag->drawTexture(sc.displayUnit);
		ag->drawMask(sc.displayUnit);
		if (renderingDebug)
			ag->drawForDebug(sc.displayUnit);
	}

	sc.displayUnit.Flush(); // make sure all drawings are sent before the "tick"
	sc.displayUnit.Tick(
	    fmt::format("Frame: {} (sent by FO # {})", frameCnt, ID));
	sc.displayUnit.Flush(); // make sure the "tick" is sent right away too

	++frameCnt;

	waitFor_renderNextFrame();
	// and we move on to get blocked on any following checkpoint
}

void FrontOfficer::reportAABBs() {
	report::message(fmt::format("I now recognize these AABBs:"));
	for (const auto& naabb : AABBs)
		report::message(fmt::format(
		    "agent ID {} \"{}\" spanning from {} to {} and living at FO #{}",
		    naabb.ID, agentsTypesDictionary.translateIdToString(naabb.nameID),
		    toString(naabb.minCorner), toString(naabb.maxCorner),
		    agentsToFOsMap[naabb.ID]));
}