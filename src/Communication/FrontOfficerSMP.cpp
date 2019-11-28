#include "../FrontOfficer.h"
#include "../Director.h"

int FrontOfficer::request_getNextAvailAgentID()
{
	return Direktor->getNextAvailAgentID();
}


void FrontOfficer::request_startNewAgent(const int newAgentID,
                                         const int associatedFO,
                                         const bool wantsToAppearInCTCtracksTXTfile)
{
	Direktor->startNewAgent(newAgentID,associatedFO,wantsToAppearInCTCtracksTXTfile);
}


void FrontOfficer::request_closeAgent(const int agentID,
                                      const int associatedFO)
{
	Direktor->closeAgent(agentID,associatedFO);
}


void FrontOfficer::request_updateParentalLink(const int childID, const int parentID)
{
	Direktor->startNewDaughterAgent(childID,parentID);
}


bool FrontOfficer::request_willRenderNextFrame()
{
	return Direktor->willRenderNextFrame();
}












//not revisited yet
void FrontOfficer::respond_publishGeometry()
{
	//round robin calls to all my agents and their publishGeometry()
}
void FrontOfficer::respond_renderNextFrame()
{
	//shall obtain images to render into
	//round robin calls to all my agents and their draw*()
	//shall use: nextFOsID
}
