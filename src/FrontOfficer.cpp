#include "FrontOfficer.h"

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
	std::list<AbstractAgent*>::iterator c=agents.begin();
	for (; c != agents.end(); c++)
	{
		(*c)->advanceAndBuildIntForces(futureTime);
#ifdef DEBUG
		if ((*c)->getLocalTime() < futureTime)
			throw ERROR_REPORT("Agent is not synchronized.");
#endif
	}

	//propagate current internal geometries to the exported ones... (can run in parallel)
	c=agents.begin();
	for (; c != agents.end(); c++)
	{
		(*c)->adjustGeometryByIntForces();
		(*c)->publishGeometry();
	}
}

void FrontOfficer::executeExternals()
{
	//react (unwillingly) to the new geometries... (can run in parallel),
	//the agents' (external at least!) geometries must not change during this phase
	std::list<AbstractAgent*>::iterator c=agents.begin();
	for (; c != agents.end(); c++)
	{
		(*c)->collectExtForces();
	}

	//propagate current internal geometries to the exported ones... (can run in parallel)
	c=agents.begin();
	for (; c != agents.end(); c++)
	{
		(*c)->adjustGeometryByExtForces();
		(*c)->publishGeometry();
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
		agents.remove(*ag);          //remove from the 'agents' list
		delete *ag;                  //remove the agent itself (its d'tor)
		ag = deadAgents.erase(ag);   //remove from the 'deadAgents' list
	}

	//register the new ones (and remove from the "new born list")
	/*
	ag = newAgents.begin();
	while (ag != newAgents.end())
	{
		agents.push_back(*ag);
		ag = newAgents.erase(ag);
	}
	*/
	//TODO: does this work the same as the commented out code above?
	agents.splice(agents.begin(), newAgents);

	//WAIT HERE UNTIL WE'RE TOLD TO START BROADCASTING OUR CHANGES
	//only FO with a "token" does broadcasting, token passing
	//is the same as for rendering/building output images (the round robin)
	waitFor_publishAgentsAABBs();

	//now distribute AABBs of the existing ones
	ag = agents.begin();
	while (ag != agents.end())
	{
		DEBUG_REPORT("reporting AABB of agent ID " << (*ag)->ID);
		broadcast_AABBofAgent(**ag);
		++ag;
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

	DEBUG_REPORT("just registered this new agent: " << ag->ID << "-" << ag->getAgentType());
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

	DEBUG_REPORT("just unregistered this dead agent: " << ag->ID << "-" << ag->getAgentType());
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


void FrontOfficer::getNearbyAgents(const ShadowAgent* const fromSA,   //reference agent
	                                const float maxDist,               //threshold dist
	                                std::list<const ShadowAgent*>& l) //output list
{
	//TODO: empty for now

	//plan:
	//scan over my AABBs to find possibly nearby agents
	//update my cache of ShadowAgents
	//populate the list with fresh references (that point
	//to the cache but the updated copies shall remain valid
	//until next update round and so do the references on them)
}


/* just in case... but for now, since relevant timing variables
   are constants, it seems that we can afford calculating the rendering
	time in isolation (each FO calculates for itself)
bool FrontOfficer::willRenderNextFrame(void)
{
	return request_willRenderNextFrame();
}
*/


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
