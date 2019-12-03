#include "../Director.h"
#include "../FrontOfficer.h"

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

/*
void Director::respond_willRenderNextFrameFlag()
{
	//gets : nothing
	//gives: bool

	//this never happens (as FO here talks directly to the Direktor)
	bool sendBackThisFlag = willRenderNextFrame();
}
*/

void Director::notify_publishAgentsAABBs(const int /* FOsID */)
{
	//notify the first FO
	//MPI_signalThisEvent_to( FOsID );

	//in the SMP world:
	FO->updateAndPublishAgents();
}


void Director::waitFor_publishAgentsAABBs()
{
	//wait here until we're told (that the broadcasting is over)

	//in MPI case, really do wait for the event to come
}


void Director::respond_AABBofAgent()
{
	//we ignore these notifications entirely
	//
	//perhaps we would see a reason later why Direktor would need
	//to know spatial relation among all agents

	//gets : AABB+type as 6x float, string
	//gives: nothing
}


size_t Director::request_CntOfAABBs(const int /* FOsID */)
{
	return FO->getSizeOfAABBsList();
}


void Director::notify_setDetailedDrawingMode(const int /* FOsID */, const int agentID, const bool state)
{
	//in the SMP world:
	FO->setAgentsDetailedDrawingMode(agentID,state);
	//
	//in the MPI, send the above to the agent FOsID
}


void Director::notify_setDetailedReportingMode(const int /* FOsID */, const int agentID, const bool state)
{
	//in the SMP world:
	FO->setAgentsDetailedReportingMode(agentID,state);
	//
	//in the MPI, send the above to the agent FOsID
}


void Director::waitHereUntilEveryoneIsHereToo()
{}

















//not revisited yet
void Director::request_publishGeometry()
{}

void Director::request_renderNextFrame(const int /* FOsID */)
{}
