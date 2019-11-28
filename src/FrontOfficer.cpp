#include "FrontOfficer.h"

void FrontOfficer::execute(void)
{
	REPORT("FO #" << ID << " is ready to start simulating");

	//main sim loop

	//this was promised to happen after every simulation round is over
	scenario.updateScene(1.0f); //TODO: time is wrong
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
}


bool FrontOfficer::willRenderNextFrame(void)
{
	return request_willRenderNextFrame();
}
