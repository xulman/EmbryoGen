#include "Agents/AbstractAgent.h"
#include "FrontOfficer.h"

void FrontOfficer::init1_SMP()
{
	REPORT("FO #" << ID << " initializing now...");
	currTime = scenario.params.constants.initTime;

	scenario.initializeScene();
	scenario.initializeAgents(this,ID,FOsCount);
}

void FrontOfficer::init2_SMP()
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


void FrontOfficer::reportSituation()
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


void FrontOfficer::close(void)
{
	//mark before closing is attempted...
	isProperlyClosedFlag = true;
	DEBUG_REPORT("running the closing sequence");

	//TODO
}


void FrontOfficer::execute(void)
{
	REPORT("FO #" << ID << " is waiting to start simulating");

	const float stopTime = scenario.params.constants.stopTime;
	const float incrTime = scenario.params.constants.incrTime;
	const float expoTime = scenario.params.constants.expoTime;

	//run the simulation rounds, one after another one
	while (currTime < stopTime)
	{
		//one simulation round is happening here,
		//will this one end with rendering?
		willRenderNextFrameFlag = currTime+incrTime >= (float)frameCnt*expoTime;

		executeInternals();
		waitHereUntilEveryoneIsHereToo();

		prepareForUpdateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		updateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		executeExternals();
		waitHereUntilEveryoneIsHereToo();

		prepareForUpdateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		updateAndPublishAgents();
		waitHereUntilEveryoneIsHereToo();

		// move to the next simulation time point
		currTime += incrTime;
		reportSituation();

		// is this the right time to export data?
		if (willRenderNextFrameFlag)
		{
			//will block itself until its part of the rendering is complete,
			//and then it'll be waiting below ready for the next loop
			renderNextFrame();
		}

		//this was promised to happen after every simulation round is over
		scenario.updateScene( currTime );
		waitHereUntilEveryoneIsHereToo();
	}
}

void FrontOfficer::executeInternals()
{
	const float incrTime = scenario.params.constants.incrTime;

	//after this simulation round is done, all agents should
	//reach local times greater than this global time
	const float futureTime = currTime + incrTime -0.0001f;

	//develop (willingly) new shapes... (can run in parallel),
	//the agents' (external at least!) geometries must not change during this phase
	std::map<int,AbstractAgent*>::iterator c=agents.begin();
	for (; c != agents.end(); c++)
	{
		c->second->advanceAndBuildIntForces(futureTime);
#ifdef DEBUG
		if (c->second->getLocalTime() < futureTime)
			throw ERROR_REPORT("Agent is not synchronized.");
#endif
	}

	//propagate current internal geometries to the exported ones... (can run in parallel)
	c=agents.begin();
	for (; c != agents.end(); c++)
	{
		c->second->adjustGeometryByIntForces();
		c->second->publishGeometry();
	}
}

void FrontOfficer::executeExternals()
{
#ifdef DEBUG
	reportAABBs();
#endif
	//react (unwillingly) to the new geometries... (can run in parallel),
	//the agents' (external at least!) geometries must not change during this phase
	std::map<int,AbstractAgent*>::iterator c=agents.begin();
	for (; c != agents.end(); c++)
	{
		c->second->collectExtForces();
	}

	//propagate current internal geometries to the exported ones... (can run in parallel)
	c=agents.begin();
	for (; c != agents.end(); c++)
	{
		c->second->adjustGeometryByExtForces();
		c->second->publishGeometry();
	}
}


void FrontOfficer::prepareForUpdateAndPublishAgents()
{
	AABBs.clear();
}

void FrontOfficer::updateAndPublishAgents()
{
	//since the last call of this method, we have:
	//list of agents that were active after the last call in 'agents'
	//subset of these that became inactive in 'deadAgents'
	//list of newly created in 'newAgents'

	//we want only to put our 'agents' list into order:
	//remove/unregister dead agents
	auto ag = deadAgents.begin();
	while (ag != deadAgents.end())
	{
		agents.erase((*ag)->ID);     //remove from the 'agents' list
		delete *ag;                  //remove the agent itself (its d'tor)
		ag = deadAgents.erase(ag);   //remove from the 'deadAgents' list
	}

	//register the new ones (and remove from the "new born list")
	ag = newAgents.begin();
	while (ag != newAgents.end())
	{
#ifdef DEBUG
		if (agents.find((*ag)->ID) != agents.end())
			throw ERROR_REPORT("Attempting to add another agent with the same ID " << (*ag)->ID);
#endif
		agents[(*ag)->ID] = *ag;
		ag = newAgents.erase(ag);
	}

	//WAIT HERE UNTIL WE'RE TOLD TO START BROADCASTING OUR CHANGES
	//only FO with a "token" does broadcasting, token passing
	//is the same as for rendering/building output images (the round robin)
	waitFor_publishAgentsAABBs();

	//now distribute AABBs of the existing ones
	for (auto ag : agents)
	{
		DEBUG_REPORT("reporting AABB of agent ID " << ag.first);
		broadcast_AABBofAgent(*(ag.second));
	}

	//this passes the "token" on another FO
	notify_publishAgentsAABBs(nextFOsID);

	//and we move on to get blocked on any following checkpoint
	//while waiting there, our respond_AABBofAgent() collects data
}


int FrontOfficer::getNextAvailAgentID()
{
	return request_getNextAvailAgentID();
}


void FrontOfficer::startNewAgent(AbstractAgent* ag, const bool wantsToAppearInCTCtracksTXTfile)
{
	if (ag == NULL)
		throw ERROR_REPORT("refuse to include NULL agent.");

	//register the agent for adding into the system:
	//local registration:
	newAgents.push_back(ag);
	ag->setOfficer(this);

	//remote registration:
	request_startNewAgent(ag->ID,this->ID,wantsToAppearInCTCtracksTXTfile);

	DEBUG_REPORT("just registered this new agent: " << IDSIGNHIM(*ag));
}


void FrontOfficer::closeAgent(AbstractAgent* ag)
{
	if (ag == NULL)
		throw ERROR_REPORT("refuse to deal with NULL agent.");

	//register the agent for removing from the system:
	//local registration
	deadAgents.push_back(ag);

	//remote registration
	request_closeAgent(ag->ID,this->ID);

	DEBUG_REPORT("just unregistered this dead agent: " << IDSIGNHIM(*ag));
}


void FrontOfficer::startNewDaughterAgent(AbstractAgent* ag, const int parentID)
{
	startNewAgent(ag, true);

	//CTC logging: also add the parental link
	request_updateParentalLink(ag->ID, parentID);
}


void FrontOfficer::closeMotherStartDaughters(
          AbstractAgent* mother,
          AbstractAgent* daughterA,
          AbstractAgent* daughterB)
{
		if (mother == NULL || daughterA == NULL || daughterB == NULL)
			throw ERROR_REPORT("refuse to deal with (some) NULL agent.");

		closeAgent(mother);
		startNewDaughterAgent(daughterA, mother->ID);
		startNewDaughterAgent(daughterB, mother->ID);
		//NB: this can be extended to any number of daughters
}


void FrontOfficer::setAgentsDetailedDrawingMode(const int agentID, const bool state)
{
	//find the agentID among currently existing agents...
	for (auto ag : agents)
	if (ag.first == agentID) ag.second->setDetailedDrawingMode(state);
}

void FrontOfficer::setAgentsDetailedReportingMode(const int agentID, const bool state)
{
	//find the agentID among currently existing agents...
	for (auto ag : agents)
	if (ag.first == agentID) ag.second->setDetailedReportingMode(state);
}


size_t FrontOfficer::getSizeOfAABBsList() const
{ return AABBs.size(); }


void FrontOfficer::getNearbyAABBs(const ShadowAgent* const fromSA,                   //reference agent
	                               const float maxDist,                               //threshold dist
	                               std::list<const NamedAxisAlignedBoundingBox*>& l)  //output list
{
	getNearbyAABBs( NamedAxisAlignedBoundingBox(fromSA->getAABB(),fromSA->getID(),fromSA->getAgentType()),
	                maxDist, l );
}


void FrontOfficer::getNearbyAABBs(const NamedAxisAlignedBoundingBox& fromThisAABB,   //reference box
	                               const float maxDist,                               //threshold dist
	                               std::list<const NamedAxisAlignedBoundingBox*>& l)  //output list
{
	const float maxDist2 = maxDist*maxDist;

	//examine all available boxes/agents
	for (const auto& b : AABBs)
	{
		//don't evaluate against itself
		if (b.ID == fromThisAABB.ID) continue;

		//close enough?
		if (fromThisAABB.minDistance(b) < maxDist2) l.push_back(&b);
	}
}


void FrontOfficer::getNearbyAgents(const ShadowAgent* const fromSA,   //reference agent
	                                const float maxDist,               //threshold dist
	                                std::list<const ShadowAgent*>& l)  //output list
{
	//list with pointers on (nearby) boxes that live in this->AABBs,
	//these were not created explicitly and so we must not delete them
	std::list<const NamedAxisAlignedBoundingBox*> nearbyBoxes;

	getNearbyAABBs( NamedAxisAlignedBoundingBox(fromSA->getAABB(),fromSA->getID(),fromSA->getAgentType()),
	                maxDist, nearbyBoxes );

	for (auto box : nearbyBoxes)
		l.push_back( getNearbyAgent(box->ID) );
}


const ShadowAgent* FrontOfficer::getNearbyAgent(const int fetchThisID)
{
	//is the requested agent living in the same (*this) FO?
	const auto ag = agents.find(fetchThisID);
	if (ag != agents.end()) return ag->second;

	DEBUG_REPORT("The requested agent " << fetchThisID << " is not living on this FO #" << ID);

	//TODO: empty for now

	//plan:
	//scan over my AABBs to find possibly nearby agents
	//update my cache of ShadowAgents
	//populate the list with fresh references (that point
	//to the cache but the updated copies shall remain valid
	//until next update round and so do the references on them)
	return NULL;
}


void FrontOfficer::renderNextFrame()
{
	REPORT("Rendering time point " << frameCnt);
	SceneControls& sc = scenario.params;

	// ----------- OUTPUT EVENTS -----------
	//clear the output images
	sc.imgMask.GetVoxelData()    = 0;
	sc.imgPhantom.GetVoxelData() = 0;
	sc.imgOptics.GetVoxelData()  = 0;

	// --------- the big round robin scheme --------- TODO
	/*
	//go over all cells, and render them
	std::list<AbstractAgent*>::const_iterator c=agents.begin();
	for (; c != agents.end(); c++)
	{
		//displayUnit should always exists in some form
		(*c)->drawTexture(displayUnit);
		(*c)->drawMask(displayUnit);
		if (renderingDebug)
			(*c)->drawForDebug(displayUnit);

		//raster images may not necessarily always exist,
		//always check for their availability first:
		if (sc.isProducingOutput(sc.imgPhantom) && sc.isProducingOutput(sc.imgOptics))
		{
			(*c)->drawTexture(sc.imgPhantom,sc.imgOptics);
		}
		if (sc.isProducingOutput(sc.imgMask))
		{
			(*c)->drawMask(sc.imgMask);
			if (renderingDebug)
				(*c)->drawForDebug(sc.imgMask); //TODO, should go into its own separate image
		}
	}
	*/

	//render the current frame
	//TODO
	/*
	displayUnit.Flush(); //make sure all drawings are sent before the "tick"
	displayUnit.Tick( ("Time: "+std::to_string(currTime)).c_str() );
	displayUnit.Flush(); //make sure the "tick" is sent right away too
	*/

	++frameCnt;
}


void FrontOfficer::reportAABBs()
{
	REPORT("I now recognize these AABBs:");
	for (const auto& naabb : AABBs)
		REPORT("agent ID " << naabb.ID << " \"" << naabb.name
		       << "\" spanning from "
		       << naabb.minCorner << " to " << naabb.maxCorner);
}
