#include "../Agents/AbstractAgent.h"
#include "../FrontOfficer.h"
#include "../Director.h"
#include "../util/strings.h"
#include <chrono>
#include <thread>

int FrontOfficer::request_getNextAvailAgentID()
{
	return communicator->getNextAvailAgentID();
}


void FrontOfficer::request_startNewAgent(const int newAgentID,
                                         const int associatedFO,
                                         const bool wantsToAppearInCTCtracksTXTfile)
{
	communicator->startNewAgent(newAgentID,associatedFO,wantsToAppearInCTCtracksTXTfile);
}


void FrontOfficer::request_closeAgent(const int agentID,
                                      const int associatedFO)
{
	communicator->closeAgent(agentID,associatedFO);
}


void FrontOfficer::request_updateParentalLink(const int childID, const int parentID)
{
	communicator->startNewDaughterAgent(childID,parentID);
}


void FrontOfficer::waitFor_publishAgentsAABBs()
{
	e_comm_tags tag = e_comm_tags::unspecified;
	for (int i = 1 ; i < communicator->getInstanceID() ; i++) {
		//communicator->appendAABBs(/*AABBs list*/);
	} 
}

void FrontOfficer::notify_publishAgentsAABBs(const int FOsID)
{
	//send the broadcasts here with  broadcast_AABBofAgent ???
	//HOW to get N? -> vzít z std::map<int,AbstractAgent*> agents  

	char data[] = {0}; // Use NULL instead? Would it be OK for MPI to work with?
	communicator->sendNextFO(data,0, e_comm_tags::send_AABB);
}


void FrontOfficer::broadcast_AABBofAgent(const ShadowAgent& ag)
{
	//this shall tell all (including this one) FOs the AABB of the given agent,
	//the Direktor actually does not care
	DEBUG_REPORT("FO #" << this->ID << " is reporting AABB of agent ID " << ag.ID);

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

	/*
	//here, according to doc/agentTypeDictionary.txt
	//we should be actually sending:
	//gives: AABB +agentID +agentTypeID +geomVersion +noOfNewAgentTypes
	//and the noOfNewAgentTypes shall be a new param of this method
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

	/*
	//here, according to doc/agentTypeDictionary.txt
	//we should actually obtain:
	//gets : AABB +agentID +agentTypeID +geomVersion +noOfNewAgentTypes
	//then:
	if (noOfNewAgentTypes > 0)
		respond_newAgentsTypes(noOfNewAgentTypes);
	*/
}


void FrontOfficer::respond_CntOfAABBs()
{
	size_t sendBackMyCount = getSizeOfAABBsList();
	communicator->sendCntOfAABBs(sendBackMyCount);
}


void FrontOfficer::broadcast_newAgentsTypes()
{
	// howManyShouldBeBroadcast()
	for (const auto& dItem : agentsTypesDictionary.theseShouldBeBroadcast())
	{
		size_t hash = dItem.first;
		hashedString::printIntoBuffer(dItem.second, __agentTypeBuf,StringsImprintSize);
		//sends away hash and agentTypeBuf
	}
}


void FrontOfficer::respond_newAgentsTypes(int noOfIncomingNewAgentTypes)
{
	//MPI world:

	//gets : N-times pairs of size_t, char[StringsImprintSize]
	//gives: nothing

	while (noOfIncomingNewAgentTypes > 0)
	{
		//fake one input:
		size_t hash = 79832748923742;
		//__agentTypeBuf = "some fake string"

		//this is how to process it:
		agentsTypesDictionary.enlistTheIncomingItem(hash,__agentTypeBuf);

		--noOfIncomingNewAgentTypes;
	}
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
	communicator->sendACKtoDirector();
}


void FrontOfficer::respond_setDetailedReportingMode()
{
	communicator->sendACKtoDirector();
}


void FrontOfficer::waitHereUntilEveryoneIsHereToo() //Will this work without specification of stage as an argument?
{
	// For cycle like in the Director to process D->FO messages? 
	int ibuffer[DIRECTOR_RECV_MAX] = {0};
	char buffer[DIRECTOR_RECV_MAX] = {0};
	int items;
	e_comm_tags tag;
	communicator->unblockNextIfSent();
	fprintf(stderr, "Running detection loop in FO %i\n", ID);
	do {
		int instance = FO_INSTANCE_ANY;
		if ((tag = communicator->detectFOMessage()) < 0) {
			// Detect other FO messages (are there any?) and broadcasts here...
			std::this_thread::sleep_for((std::chrono::milliseconds)10);
			continue;
		}
		switch (tag) {
			case e_comm_tags::set_detailed_drawing:
				communicator->receiveDirectorMessage(buffer, items, tag);
				assert(items == 2);
				setAgentsDetailedDrawingMode(buffer[0], buffer[1] != 0);
				respond_setDetailedDrawingMode();
				break;
			case e_comm_tags::set_detailed_reporting:
				communicator->receiveDirectorMessage(buffer, items, tag);
				assert(items == 2);
				setAgentsDetailedReportingMode(buffer[0], buffer[1] != 0);
				respond_setDetailedReportingMode();
				break;
			case e_comm_tags::send_AABB:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 0);
				//Is something else called here?
				notify_publishAgentsAABBs(nextFOsID);
				break;
			case e_comm_tags::render_frame:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 0);
				//Is something else called here?
				request_renderNextFrame(nextFOsID);
				break;
			case e_comm_tags::next_stage:
				communicator->receiveFOMessage(buffer, items, instance, tag);
				break;
			default:
				 fprintf(stderr,"Unprocessed communication tag %i on FO %i\n", tag, ID);
				 communicator->receiveFOMessage(buffer, items, instance, tag);
				 break;
		}
		
	} while (tag != e_comm_tags::next_stage);
	fprintf(stderr, "Ending detection loop in FO %i and waiting for sync\n", ID);
	communicator->waitSync(); //waitFor_publishAgentsAABBs();
}


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
	communicator->waitFor_renderNextFrame();
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
	// To Director only?
	//MPI world:

	//gets : nothing
	//gives: char*

	//just non-blocking broadcast of the exceptionMessage, don't throw
	//it here because this should only be called already from the catch(),
	//that is the exception has already been thrown...
}


void FrontOfficer::respond_throwException()
{
	// To Director only?
	//MPI world:

	//gets : bool
	//gives: nothing

	//read the exceptionMessage, then throw it
	char msg[] = "received this fake exception message";
	throw new std::runtime_error( msg );
}
