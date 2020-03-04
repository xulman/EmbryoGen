#include "../Agents/AbstractAgent.h"
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

/*
bool FrontOfficer::request_willRenderNextFrame()
{
	return Direktor->willRenderNextFrame();
}
*/

void FrontOfficer::waitFor_publishAgentsAABBs()
{
	//wait here until we're told to start the broadcast which
	//our agents were removed and AABBs of the remaining ones

	//in MPI case, really do wait for the event to come
}

void FrontOfficer::notify_publishAgentsAABBs(const int /* FOsID */)
{
	//once our broadcasting is over, notify next FO to do the same
	//(or notify the Direktor if we're the last on the round-robin-chain)

	//in this SMP particular implementation we do nothing because
	//there is no other FO, and the Direktor called us directly

	//in MPI case, tell the next FO (or the Direktor)
	//MPI_signalThisEvent_to( FOsID );
}


void FrontOfficer::broadcast_AABBofAgent(const ShadowAgent& ag)
{
	//this shall tell all (including this one) FOs the AABB of the given agent,
	//the Direktor actually does not care

	//in this SMP particular implementation we do only update ourselves,
	//there is no other FO and Direktor doesn't care about this update
	//
	//this code essentially supplies the work of respond_AABBofAgent() in the SMP world
	AABBs.emplace_back(ag.getAABB(),ag.getID(),ag.getAgentType());
	agentsAndBroadcastGeomVersions[ag.getID()] = ag.getGeometry().version;
	registerThatThisAgentIsAtThisFO(ag.getID(),this->ID);

	/*
	//in MPI world: example:
	float coords[] = {ag.getAABB().minCorner.x,
	                  ag.getAABB().minCorner.y,
	                  ag.getAABB().minCorner.z,
	                  ag.getAABB().maxCorner.x,
	                  ag.getAABB().maxCorner.y,
	                  ag.getAABB().maxCorner.z};
	//important, also transfer agent's ID and type/string:
	ag.getID();
	ag.getAgentType();
	int sendVersion = ag.getGeometry().version;
	//send out 6x float, int, string, int --> all to be received by respond_AABBofAgent()

	//TODO: check that the broadcast reaches myself too!
	//      or just update myself explicitly just like above in the SMP case
	*/
}


void FrontOfficer::respond_AABBofAgent()
{
	//this never happens (as there's no other FO to call us)
	//MPI world:

	//gets : AABB +ID +type +geomVersion as 6x float, int, string, int
	//gives: nothing
	//also needs to get the ID of the FO that broadcasted that particular message
	//(if that is not possible, the sending FOsID will need be part of the message)

	/*
	//fake example values:
	int FOsIDfromWhichTheMessageArrived(1);
	//
	float coords[] = { 10.f,10.f,10.f, 20.f,20.f,20.f };
	int         agentID(10);
	std::string agentType("some fictious agent");
	int         geomVersion(42);

	AABBs.emplace_back();
	AABBs.back().minCorner.fromScalars(coords[0],coords[1],coords[2]);
	AABBs.back().maxCorner.fromScalars(coords[3],coords[4],coords[5]);
	AABBs.back().ID   = agentID;
	AABBs.back().name = agentType;
	agentsAndBroadcastGeomVersions[agentID] = geomVersion;
	registerThatThisAgentIsAtThisFO(agentID,FOsIDfromWhichTheMessageArrived);
	*/
}


void FrontOfficer::respond_CntOfAABBs()
{
	//this never happens (as Direktor here talks directly to the FO)
	//MPI world:

	//gets : nothing
	//gives: int

	/*
	size_t sendBackMyCount = getSizeOfAABBsList();
	*/
}


ShadowAgent* FrontOfficer::request_ShadowAgentCopy(const int /* agentID */, const int /* FOsID */)
{
	//this never happens here (as there is no other FO to talk to)
	return NULL;

	//MPI world:
	//contacts FO at FOsID telling it that it needs agentID's ShadowAgent back

	//gives: agentID, this->ID (to tell the other party whom to talk back)
	//waits
	//gets : Geometry data, agentID and agentType

	/* fake data!
	Geometry*   gotThisGeom      = new Spheres(4);
	int         gotThisAgentID   = 10;
	std::string gotThisAgentType = "fake agent";
	return new ShadowAgent(*gotThisGeom, gotThisAgentID,gotThisAgentType);
	*/
}


void FrontOfficer::respond_ShadowAgentCopy()
{
	//this never happens here (as there is no other FO to hear from)
	//MPI world:

	//gets : requestedAgentID, askingFOsID
	//gives: Geometry data, agentID and agentType

	/*
	int requestedAgentID = 10; //fake value

#ifdef DEBUG
	if (agents.find(requestedAgentID) == agents.end())
		throw ERROR_REPORT("Cannot provide ShadowAgent for agent ID " << requestedAgentID);
#endif

	const AbstractAgent& aaRef = *(agents[requestedAgentID]);

	const Geometry&    sendBackGeom      = aaRef.getGeometry();
	const int          sendBackAgentID   = aaRef.getID();
	const std::string& sendBackAgentType = aaRef.getAgentType();
	*/
}


void FrontOfficer::respond_setDetailedDrawingMode()
{
	//this never happens (as Direktor here talks directly to the FO)
	//MPI world:

	//gets : int, bool
	//gives: nothing

	/*
	int agentID = 1;
	bool state = false;
	setAgentsDetailedDrawingMode(agentID,state);
	*/
}


void FrontOfficer::respond_setDetailedReportingMode()
{
	//this never happens (as Direktor here talks directly to the FO)
	//MPI world:

	//gets : int, bool
	//gives: nothing

	/*
	int agentID = 1;
	bool state = false;
	setAgentsDetailedReportingMode(agentID,state);
	*/
}


void FrontOfficer::waitHereUntilEveryoneIsHereToo()
{}


void FrontOfficer::waitFor_renderNextFrame()
{
	//normally, it should move the content of the images from this FO
	//into Direktor's images... but instead we render directly into
	//Direktor's images... see FrontOfficer::renderNextFrame()
}


void FrontOfficer::request_renderNextFrame(const int /* FOsID */)
{
	//does nothing in the SMP

	//MPI world:

	//this is the counterpart to the waitFor_renderNextFrame()
	//one sends out the images in this order: mask, phantom, optics
}


void FrontOfficer::respond_setRenderingDebug()
{
	//this never happens (as Direktor here talks directly to the FO)
	//MPI world:

	//gets : bool
	//gives: nothing

	/*
	bool gotThisFlagValue = false; //fake value here!
	setSimulationDebugRendering(gotThisFlagValue);
	*/
}


void FrontOfficer::broadcast_throwException(const char* /* exceptionMessage */)
{
	//this never happens (as Direktor and FO live in the same try block)
	//MPI world:

	//gets : nothing
	//gives: char*

	//just non-blocking broadcast of the exceptionMessage, don't throw
	//it here because this should only be called already from the catch(),
	//that is the exception has already been thrown...
}


void FrontOfficer::respond_throwException()
{
	//this never happens (as Direktor and FO live in the same try block)
	//MPI world:

	//gets : bool
	//gives: nothing

	//read the exceptionMessage, then throw it
	char msg[] = "received this fake exception message";
	throw new std::runtime_error( msg );
}
