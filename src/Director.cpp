#include "Director.h"

void Director::execute(void)
{
	//main sim loop
	bool renderingNow = true;
	request_publishGeometry();

	if (renderingNow)
	{
		request_renderNextFrame();
		scenario.doPhaseIIandIII();
	}

	//this was promised to happen after every simulation round is over
	scenario.updateScene(1.0f); //TODO: time is wrong
}


int Director::getNextAvailAgentID()
{
	return ++lastUsedAgentID;
}


void Director::startNewAgent(const int agentID,
                             const int associatedFO,
                             const bool wantsToAppearInCTCtracksTXTfile)
{
	//register the agent for adding into the system
	newAgents.emplace_back(agentID,associatedFO);

	//CTC logging?
	if (wantsToAppearInCTCtracksTXTfile)
		tracks.startNewTrack(agentID, frameCnt);
}


void Director::closeAgent(const int agentID,
                          const int associatedFO)
{
	//register the agent for removing from the system
	deadAgents.emplace_back(agentID,associatedFO);

	//CTC logging?
	if (tracks.isTrackFollowed(agentID))
		tracks.closeTrack(agentID, frameCnt-1);
}


void Director::startNewDaughterAgent(const int childID, const int parentID)
{
	//CTC logging: also add the parental link
	tracks.updateParentalLink(childID, parentID);
}
