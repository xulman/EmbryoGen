#include "../Agents/AbstractAgent.h"
#include "../FrontOfficer.h"
#include "../Director.h"

int FrontOfficer::request_getNextAvailAgentID()
{
	//return Direktor->getNextAvailAgentID();
	//blocks and waits until it gets int back from the Director
	return 1;
}


void FrontOfficer::request_startNewAgent(const int newAgentID,
                                         const int associatedFO,
                                         const bool wantsToAppearInCTCtracksTXTfile)
{
	//Direktor->startNewAgent(newAgentID,associatedFO,wantsToAppearInCTCtracksTXTfile);
	//may block and wait until Direktor confirms the request was processed
}


void FrontOfficer::request_closeAgent(const int agentID,
                                      const int associatedFO)
{
	//Direktor->closeAgent(agentID,associatedFO);
	//may block and wait until Direktor confirms the request was processed
}


void FrontOfficer::request_updateParentalLink(const int childID, const int parentID)
{
	//Direktor->startNewDaughterAgent(childID,parentID);
	//may block and wait until Direktor confirms the request was processed
}


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

	/*
	//in MPI world: example:
	float coords[] = {ag.getAABB().minCorner.x,
	                  ag.getAABB().minCorner.y,
	                  ag.getAABB().minCorner.z,
	                  ag.getAABB().maxCorner.x,
	                  ag.getAABB().maxCorner.y,
	                  ag.getAABB().maxCorner.z};
	//important, also transfer agent's ID and typeID:
	ag.getID();
	ag.getAgentTypeID();
	int sendVersion = ag.getGeometry().version;
	//send out 6x float, int, size_t, int --> all to be received by respond_AABBofAgent()

	//TODO: check that the broadcast reaches myself too!
	//      or just update myself explicitly just like above in the SMP case
	*/
}


void FrontOfficer::respond_AABBofAgent()
{
	//MPI world:

	//gets : AABB +agentID +agentTypeID +geomVersion as 6x float, int, size_t, int
	//gives: nothing
	//also needs to get the ID of the FO that broadcasted that particular message
	//(if that is not possible, the sending FOsID will need be part of the message)

	/*
	//fake example values:
	int FOsIDfromWhichTheMessageArrived(1);
	//
	float coords[] = { 10.f,10.f,10.f, 20.f,20.f,20.f };
	int         agentID(10);
	size_t      agentTypeID(20);
	int         geomVersion(42);

	AABBs.emplace_back();
	AABBs.back().minCorner.fromScalars(coords[0],coords[1],coords[2]);
	AABBs.back().maxCorner.fromScalars(coords[3],coords[4],coords[5]);
	AABBs.back().ID     = agentID;
	AABBs.back().nameID = agentTypeID;
	agentsAndBroadcastGeomVersions[agentID] = geomVersion;
	registerThatThisAgentIsAtThisFO(agentID,FOsIDfromWhichTheMessageArrived);
	*/
}


void FrontOfficer::respond_CntOfAABBs()
{
	//MPI world:

	//gets : nothing
	//gives: int

	/*
	size_t sendBackMyCount = getSizeOfAABBsList();
	*/
}


ShadowAgent* FrontOfficer::request_ShadowAgentCopy(const int /* agentID */, const int /* FOsID */)
{
	//faked return!
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
	//MPI world:

	//shall block and wait until 3 images come to us
	//they will come in this order: mask, phantom, optics

#ifdef DEBUG
	//test this:
	//the incomming images must be of the same size as ours sc.img*
#endif

	//one receives each image directly into our own sc.img* image,
	//to "receive directly" is to sum (operation +) together corresponding
	//pixels, that is pixel from the network with pixel in the sc.img*,
	//the result is stored into the sc.img*

	/*
	SceneControls& sc = scenario.params;

	//here's the handle on our sc.img* images
	sc.imgMask
	sc.imgPhantom
	sc.imgOptics

	//here's how to reach the pixels, they live in one long buffer, e.g.
	i3d::GRAY16* maskPixelBuffer = sc.imgMask.GetFirstVoxelAddr();
	const size_t maskPixelLength = sc.imgMask.GetImageSize();

	//reminder of the types of the images:
	i3d::Image3d<i3d::GRAY16> imgMask;
	i3d::Image3d<float> imgPhantom;
	i3d::Image3d<float> imgOptics;

	(i3d::GRAY16 maps into short type)
	*/
}


void FrontOfficer::request_renderNextFrame(const int /* FOsID */)
{
	//MPI world:

	//this is the counterpart to the waitFor_renderNextFrame()
	//one sends out the images in this order: mask, phantom, optics
}


void FrontOfficer::respond_setRenderingDebug()
{
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
	//MPI world:

	//gets : nothing
	//gives: char*

	//just non-blocking broadcast of the exceptionMessage, don't throw
	//it here because this should only be called already from the catch(),
	//that is the exception has already been thrown...
}


void FrontOfficer::respond_throwException()
{
	//MPI world:

	//gets : bool
	//gives: nothing

	//read the exceptionMessage, then throw it
	char msg[] = "received this fake exception message";
	throw new std::runtime_error( msg );
}
