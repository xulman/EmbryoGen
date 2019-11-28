#include "../Director.h"

//in the SMP world, a lot of respond_...() methods are actually
//empty because the FOs reach directly the implementing code

void Director::respond_getNextAvailAgentID()
{
	//gets : nothing
	//gives: int

	//this never happens (as FO here talks directly to the Direktor)
	int sendBackThisNewID = getNextAvailAgentID();
}


void Director::respond_startNewAgent()
{
	//gets : int, int, bool
	//gives: nothing

	//have to learn from the other party:
	int newAgentID;
	int associatedFO;
	bool wantsToAppearInCTCtracksTXTfile;

	startNewAgent(newAgentID,associatedFO,wantsToAppearInCTCtracksTXTfile);
}


void Director::respond_closeAgent()
{
	//gets : int, int
	//gives: nothing

	//have to learn from the other party:
	int agentID;
	int associatedFO;

	closeAgent(agentID,associatedFO);
}


void Director::respond_updateParentalLink()
{
	//gets : int, int
	//gives: nothing

	//have to learn from the other party:
	int childID;
	int parentID;

	startNewDaughterAgent(childID,parentID);
}


void Director::respond_willRenderNextFrameFlag()
{
	//gets : nothing
	//gives: bool

	//this never happens (as FO here talks directly to the Direktor)
	bool willRenderNextFrameFlag = willRenderNextFrame();
}

















//not revisited yet
void Director::request_publishGeometry()
{}

void Director::request_renderNextFrame()
{}
