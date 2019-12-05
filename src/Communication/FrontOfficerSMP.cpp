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


/*
 * TODO:
 * after the push-broadcast round of AABBs it should be clear
 * which agents are now active in the simulation, and
 * every agent decides who is nearby and requests its detailed
 * geometry.. FOs then cache this requests
 * to avoid re-fetching of heavy geometries (e.g. static
 * scalarImg geometries), we could introduce a version number
 * in the geometry and FO, when asking for detailed geometry,
 * would include the version it holds and replier would either
 * confirm "nothing new since then" or would just sent newer version
 */


void FrontOfficer::broadcast_AABBofAgent(const ShadowAgent& ag)
{
	//this shall tell all (including this one) FOs the AABB of the given agent,
	//the Direktor actually does not care

	//in this SMP particular implementation we do only update ourselves,
	//there is no other FO and Direktor doesn't care about this update
	AABBs.emplace_back(ag.getAABB(),ag.getID(),ag.getAgentType());
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
	//send out 6x float, int, string - to be received by respond_AABBofAgent()

	//TODO: check that the broadcast reaches myself too!
	//      or just update myself explicitly just like above in the SMP case
	*/
}


void FrontOfficer::respond_AABBofAgent()
{
	//this never happens (as there's no other FO to call us)
	//MPI world:

	//gets : AABB+ID+type as 6x float, int, string
	//gives: nothing
	//also needs to get the ID of the FO that broadcasted that particular message
	//(if that is not possible, the sending FOsID will need be part of the message)

	/*
	//fake example values:
	float coords[] = { 10.f,10.f,10.f, 20.f,20.f,20.f };
	int agentID(10);
	int FOsIDfromWhichTheMessageArrived(1);
	std::string agentType("some fictious agent");

	AABBs.emplace_back();
	AABBs.back().minCorner.fromScalars(coords[0],coords[1],coords[2]);
	AABBs.back().maxCorner.fromScalars(coords[3],coords[4],coords[5]);
	AABBs.back().ID   = agentID;
	AABBs.back().name = agentType;

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










//not revisited yet
void FrontOfficer::respond_renderNextFrame()
{
	//shall obtain images to render into
	//round robin calls to all my agents and their draw*()
	//shall use: nextFOsID
}
