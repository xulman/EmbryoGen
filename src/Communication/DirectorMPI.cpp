#include "../Director.h"
#include "../FrontOfficer.h"

//in the SMP world, a lot of respond_...() methods are actually
//empty because the FOs reach directly the implementing code

void Director::respond_getNextAvailAgentID()
{
	//MPI world:

	//gets : nothing
	//gives: int

	/*
	int sendBackThisNewID = getNextAvailAgentID();
	*/
}


void Director::respond_startNewAgent()
{
	//MPI world:

	//gets : int, int, bool
	//gives: nothing

	//have to learn from the other party:
	/*
	int newAgentID;
	int associatedFO;
	bool wantsToAppearInCTCtracksTXTfile;

	startNewAgent(newAgentID,associatedFO,wantsToAppearInCTCtracksTXTfile);
	*/
}


void Director::respond_closeAgent()
{
	//MPI world:

	//gets : int, int
	//gives: nothing

	//have to learn from the other party:
	/*
	int agentID;
	int associatedFO;

	closeAgent(agentID,associatedFO);
	*/
}


void Director::respond_updateParentalLink()
{
	//MPI world:

	//gets : int, int
	//gives: nothing

	//have to learn from the other party:
	/*
	int childID;
	int parentID;

	startNewDaughterAgent(childID,parentID);
	*/
}


void Director::notify_publishAgentsAABBs(const int /* FOsID */)
{
	//notify the first FO
	//MPI_signalThisEvent_to( FOsID );
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

	//gets : AABB+type as 6x float, int, string
	//gives: nothing
}


size_t Director::request_CntOfAABBs(const int /* FOsID */)
{
	//block and wait until size_t comes back from the front officer FOsID
	return 10; //faked value!
}


void Director::notify_setDetailedDrawingMode(const int /* FOsID */, const int agentID, const bool state)
{
	//in the SMP world:
	//FO->setAgentsDetailedDrawingMode(agentID,state);
	//
	//in the MPI, send the above to the front officer FOsID
}


void Director::notify_setDetailedReportingMode(const int /* FOsID */, const int agentID, const bool state)
{
	//in the SMP world:
	//FO->setAgentsDetailedReportingMode(agentID,state);
	//
	//in the MPI, send the above to the agent FOsID
}


void Director::waitHereUntilEveryoneIsHereToo()
{}


void Director::request_renderNextFrame(const int /* FOsID */)
{
	//in the SMP:
	//FO->renderNextFrame();

	//MPI world:
	//this exactly the same code as in FrontOfficer::request_renderNextFrame(FOsID)
}


void Director::waitFor_renderNextFrame()
{
	//MPI world:
	//this exactly the same code as in FrontOfficer::waitFor_renderNextFrame()
}


void Director::broadcast_setRenderingDebug(const bool setFlagToThis)
{
	//SMP:
	//FO->setSimulationDebugRendering(setFlagToThis);

	//MPI world:
	//non-blocking send out to everyone
}


void Director::broadcast_throwException(const char* /* exceptionMessage */)
{
	//MPI world:
	//the same as FrontOfficer::broadcast_throwException(exceptionMessage)
}


void Director::respond_throwException()
{
	//MPI world:
	//the same as FrontOfficer::respond_throwException()
}
