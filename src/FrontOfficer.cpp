#include "FrontOfficer.hpp"
#include "Agents/AbstractAgent.hpp"
#include "Director.hpp"

// Collect external forces

// Collect external forces

void FrontOfficer::init1_SMP() {
	report::message(fmt::format("FO #{} initializing now...", ID));
	currTime = scenario.params->constants.initTime;

	scenario.initializeScene();
	scenario.initializeAgents(this, ID, FOsCount);
}

void FrontOfficer::init2_SMP() {
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

void FrontOfficer::close(void) {
	// mark before closing is attempted...
	isProperlyClosedFlag = true;
	report::debugMessage(fmt::format("running the closing sequence"));

	// TODO: should close/kill the service thread too

	// delete all agents... also later from newAgents & deadAgents, note that
	// the same agent may exist on the agents and deadAgents lists
	// simultaneously
	report::debugMessage(
	    fmt::format("will remove {} active agents", agents.size()));
	for (auto ag : agents) {
		// check and possibly remove from the deadAgents list
		auto daIt = deadAgents.begin();
		while (daIt != deadAgents.end() && *daIt != ag.second)
			++daIt;
		if (daIt != deadAgents.end()) {
			report::debugMessage(fmt::format(
			    "removing from deadAgents list the duplicate agent ID {}",
			    (*daIt)->ID));
			deadAgents.erase(daIt);
		}

		delete ag.second;
		ag.second = NULL;
	}
	agents.clear();

	// now remove what's left on newAgents and deadAgents
	report::debugMessage(fmt::format("will remove {} and {} agents from "
	                                 "newAgents and deadAgents, respectively",
	                                 newAgents.size(), deadAgents.size()));
	while (!newAgents.empty()) {
		delete newAgents.front();
		newAgents.pop_front();
	}
	while (!deadAgents.empty()) {
		delete deadAgents.front();
		deadAgents.pop_front();
	}

	// also clean up any shadow agents one may have
	report::debugMessage(
	    fmt::format("will remove {} shadow agents", shadowAgents.size()));
	for (auto sh : shadowAgents) {
		delete sh.second;
		sh.second = NULL;
	}
	shadowAgents.clear();

	// clean up aux attribs
	delete[] __agentTypeBuf;
}

void FrontOfficer::execute(void) {
	report::message(fmt::format("FO #{} is waiting to start simulating", ID));

	const float stopTime = scenario.params->constants.stopTime;
	const float incrTime = scenario.params->constants.incrTime;
	const float expoTime = scenario.params->constants.expoTime;

	// run the simulation rounds, one after another one
	while (currTime < stopTime) {
		// one simulation round is happening here,
		// will this one end with rendering?
		willRenderNextFrameFlag =
		    currTime + incrTime >= (float)frameCnt * expoTime;

		executeInternals();
		waitHereUntilEveryoneIsHereToo();

		prepareForUpdateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		updateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();
		postprocessAfterUpdateAndPublishAgents();

		executeExternals();
		waitHereUntilEveryoneIsHereToo();

		prepareForUpdateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		updateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();
		postprocessAfterUpdateAndPublishAgents();

		// move to the next simulation time point
		executeEndSub1();

		// is this the right time to export data?
		if (willRenderNextFrameFlag) {
			// will block itself until its part of the rendering is complete,
			// and then it'll be waiting below ready for the next loop
			renderNextFrame();
		}

		executeEndSub2();
		waitHereUntilEveryoneIsHereToo();
	}
}

void FrontOfficer::executeEndSub1() {
	// move to the next simulation time point
	currTime += scenario.params->constants.incrTime;
	reportSituation();
}

void FrontOfficer::executeEndSub2() {
	// this was promised to happen after every simulation round is over
	scenario.updateScene(currTime);
}

void FrontOfficer::executeInternals() {
	// after this simulation round is done, all agents should
	// reach local times greater than this global time
	const float futureTime =
	    currTime + scenario.params->constants.incrTime - 0.0001f;

	// develop (willingly) new shapes... (can run in parallel),
	// the agents' (external at least!) geometries must not change during this
	// phase
	std::map<int, AbstractAgent*>::iterator c = agents.begin();
	for (; c != agents.end(); c++) {
		c->second->advanceAndBuildIntForces(futureTime);
#ifndef NDEBUG
		if (c->second->getLocalTime() < futureTime)
			throw report::rtError("Agent is not synchronized.");
#endif
	}

	// propagate current internal geometries to the exported ones... (can run in
	// parallel)
	c = agents.begin();
	for (; c != agents.end(); c++) {
		c->second->adjustGeometryByIntForces();
		c->second->publishGeometry();
	}
}

void FrontOfficer::executeExternals() {
#ifndef NDEBUG
	reportAABBs();
#endif
	// react (unwillingly) to the new geometries... (can run in parallel),
	// the agents' (external at least!) geometries must not change during this
	// phase
	std::map<int, AbstractAgent*>::iterator c = agents.begin();
	for (; c != agents.end(); c++) {
		c->second->collectExtForces();
	}

	// propagate current internal geometries to the exported ones... (can run in
	// parallel)
	c = agents.begin();
	for (; c != agents.end(); c++) {
		c->second->adjustGeometryByExtForces();
		c->second->publishGeometry();
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
	auto ag = deadAgents.begin();
	while (ag != deadAgents.end()) {
		agents.erase((*ag)->ID);   // remove from the 'agents' list
		delete *ag;                // remove the agent itself (its d'tor)
		ag = deadAgents.erase(ag); // remove from the 'deadAgents' list
	}

	// register the new ones (and remove from the "new born list")
	ag = newAgents.begin();
	while (ag != newAgents.end()) {
#ifndef NDEBUG
		if (agents.find((*ag)->ID) != agents.end())
			throw report::rtError(fmt::format(
			    "Attempting to add another agent with the same ID {}",
			    (*ag)->ID));
#endif
		agents[(*ag)->ID] = *ag;
		ag = newAgents.erase(ag);
	}

	// WAIT HERE UNTIL WE'RE TOLD TO START BROADCASTING OUR CHANGES
	// only FO with a "token" does broadcasting, token passing
	// is the same as for rendering/building output images (the round robin)
	waitFor_publishAgentsAABBs();

	// now distribute AABBs of the existing ones
	for (auto ag : agents) {
		broadcast_AABBofAgent(*(ag.second));
	}

	/*
	//here, according to doc/agentTypeDictionary.txt
	//the previous for-cycle would look like this:
	size_t cnt = agents.size();
	for (auto ag : agents)
	{
report::debugMessage(fmt::format("reporting AABB of agent ID {}" , ag.first));
	    if (cnt > 1)
	        broadcast_AABBofAgent(*(ag.second),0);
	    else
	        //the last AABB reports how many agentsTypesDictionary items will
follow
	        broadcast_AABBofAgent(*(ag.second),agentsTypesDictionary.howManyShouldBeBroadcast());
	    --cnt;
	}
	*/

	// this passes the "token" on another FO
	notify_publishAgentsAABBs(nextFOsID);

	// and we move on to get blocked on any following checkpoint
	// while waiting there, our respond_AABBofAgent() collects data
}

void FrontOfficer::postprocessAfterUpdateAndPublishAgents() {
	// post-process local Dictionary
	agentsTypesDictionary.markAllWasBroadcast();
	agentsTypesDictionary.cleanUp(AABBs);
}

int FrontOfficer::getNextAvailAgentID() {
	return request_getNextAvailAgentID();
}

void FrontOfficer::startNewAgent(AbstractAgent* ag,
                                 const bool wantsToAppearInCTCtracksTXTfile) {
	if (ag == NULL)
		throw report::rtError("refuse to include NULL agent.");

	// register the agent for adding into the system:
	// local registration:
	agentsTypesDictionary.registerThisString(ag->getAgentType_hashedString());
	newAgents.push_back(ag);
	ag->setOfficer(this);

	// remote registration:
	request_startNewAgent(ag->ID, this->ID, wantsToAppearInCTCtracksTXTfile);

	report::debugMessage(
	    fmt::format("just registered this new agent: {}", ag->getSignature()));
}

void FrontOfficer::closeAgent(AbstractAgent* ag) {
	if (ag == NULL)
		throw report::rtError(fmt::format("refuse to deal with NULL agent."));

	// register the agent for removing from the system:
	// local registration
	deadAgents.push_back(ag);

	// remote registration
	request_closeAgent(ag->ID, this->ID);

	report::debugMessage(fmt::format("just unregistered this dead agent: {}",
	                                 ag->getSignature()));
}

void FrontOfficer::startNewDaughterAgent(AbstractAgent* ag,
                                         const int parentID) {
	startNewAgent(ag, true);

	// CTC logging: also add the parental link
	request_updateParentalLink(ag->ID, parentID);
}

void FrontOfficer::closeMotherStartDaughters(AbstractAgent* mother,
                                             AbstractAgent* daughterA,
                                             AbstractAgent* daughterB) {
	if (mother == NULL || daughterA == NULL || daughterB == NULL)
		throw report::rtError("refuse to deal with (some) NULL agent.");

	closeAgent(mother);
	startNewDaughterAgent(daughterA, mother->ID);
	startNewDaughterAgent(daughterB, mother->ID);
	// NB: this can be extended to any number of daughters
}

void FrontOfficer::setAgentsDetailedDrawingMode(const int agentID,
                                                const bool state) {
	// find the agentID among currently existing agents...
	for (auto ag : agents)
		if (ag.first == agentID)
			ag.second->setDetailedDrawingMode(state);
}

void FrontOfficer::setAgentsDetailedReportingMode(const int agentID,
                                                  const bool state) {
	// find the agentID among currently existing agents...
	for (auto ag : agents)
		if (ag.first == agentID)
			ag.second->setDetailedReportingMode(state);
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
		return ag->second;

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
	auto const saItem = shadowAgents.find(fetchThisID);
	int storedVersion = (saItem != shadowAgents.end())
	                        ? saItem->second->getGeometry().version
	                        : -1000000;

	// if we have a recent geometry by us, let's just return this one
	if (storedVersion == agentsAndBroadcastGeomVersions[fetchThisID])
		return saItem->second;

	// else, we have to obtain the most recent copy...
	const int contactThisFO = agentsToFOsMap[fetchThisID];
	report::debugMessage(fmt::format("Requesting agent ID {} from FO #{}",
	                                 fetchThisID, contactThisFO));
	ShadowAgent* const saCopy =
	    request_ShadowAgentCopy(fetchThisID, contactThisFO);

	// delete the now-old content first (if there was some)
	if (saItem != shadowAgents.end())
		delete saItem->second;

	// store the new reference
	shadowAgents[fetchThisID] = saCopy;

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

	return saCopy;
}

void FrontOfficer::renderNextFrame() {
	report::message(fmt::format("Rendering time point {}", frameCnt));
	SceneControls& sc = *scenario.params;

	// ----------- OUTPUT EVENTS -----------
	// clear the output images
	sc.imgMask.GetVoxelData() = 0;
	sc.imgPhantom.GetVoxelData() = 0;
	sc.imgOptics.GetVoxelData() = 0;

	// go over all cells, and render them -- ONLY IMAGES!
	for (auto ag : agents) {
		// raster images may not necessarily always exist,
		// always check for their availability first:
		if (sc.isProducingOutput(sc.imgPhantom) &&
		    sc.isProducingOutput(sc.imgOptics)) {
			ag.second->drawTexture(Direktor->refOnDirektorsImgPhantom(),
			                       Direktor->refOnDirektorsImgOptics());
		}
		if (sc.isProducingOutput(sc.imgMask)) {

			ag.second->drawMask(Direktor->refOnDirektorsImgMask());
			if (renderingDebug)
				ag.second->drawForDebug(
				    Direktor
				        ->refOnDirektorsImgMask()); // TODO, should go into its
				                                    // own separate image
		}
	}
	// note that this far the code was executed on all FOs, that means in
	// parallel

	// --------- the big round robin scheme ---------
	// WAIT HERE UNTIL WE GET THE IMAGES TO CONTRIBUTE INTO
	// this will block...
	waitFor_renderNextFrame(nextFOsID);

	// note that it is only us who have the token, we
	// do "pollute" the DisplayUnit (load balancing)
	//
	// go over all cells, and render them -- ONLY DISPLAY UNITS!
	for (auto ag : agents) {
		ag.second->drawTexture(sc.displayUnit);
		ag.second->drawMask(sc.displayUnit);
		if (renderingDebug)
			ag.second->drawForDebug(sc.displayUnit);
	}

	sc.displayUnit.Flush(); // make sure all drawings are sent before the "tick"
	sc.displayUnit.Tick(
	    fmt::format("Frame: {} (sent by FO # {})", frameCnt, ID));
	sc.displayUnit.Flush(); // make sure the "tick" is sent right away too

	++frameCnt;

	request_renderNextFrame(nextFOsID);
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
