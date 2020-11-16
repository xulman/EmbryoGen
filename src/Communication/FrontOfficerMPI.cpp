#include "../Agents/AbstractAgent.h"
#include "../FrontOfficer.h"
#include "../Director.h"
#include "../util/strings.h"
#include "DistributedCommunicator.h"
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
	e_comm_tags tag = e_comm_tags::send_AABB;
	DEBUG_REPORT("FO #" << this->ID << " is running AABB reporting cycle with local size " << agents.size());
	t_aabb * sentAABBs;
	for (int i = 1 ; i <= FOsCount ; i++) {
		if (i == ID)
		{
			int aabb_count = agents.size(); //getSizeOfAABBsList(); is not returning correct results! (WHY? -> Because the AABB list is empty)
			/* TBD: In future, will local AABB count in each FO always fit into integer? Otherwise we have a problem */
			int j = 0;
			communicator->sendCntOfAABBs(aabb_count, true);
			sentAABBs = new t_aabb[aabb_count];
			for (auto ag : agents)
			{
				const AxisAlignedBoundingBox & aabb = ag.second->getAABB();
				//REPORT("Agent #" << j << " is running AABB reporting cycle with local size " << agents.size());
				sentAABBs[j].minCorner = aabb.minCorner;
				sentAABBs[j].maxCorner = aabb.maxCorner;
				sentAABBs[j].version = ag.second->getGeometry().version;
				sentAABBs[j].id = ag.second->getID();
				sentAABBs[j].atype = ag.second->getAgentTypeID();
				j++;
			}
			communicator->sendBroadcast(sentAABBs, aabb_count, i, e_comm_tags::send_AABB);
			delete sentAABBs;
		}
		else
		{
			int aabb_count = communicator->cntOfAABBs(i, true);
			sentAABBs = new t_aabb[aabb_count];
			communicator->receiveBroadcast(sentAABBs, aabb_count, i, e_comm_tags::send_AABB);
			for (int j=0; j < aabb_count; j++) {
				AABBs.emplace_back();
				AABBs.back().minCorner = sentAABBs[j].minCorner;
				AABBs.back().maxCorner = sentAABBs[j].maxCorner;
				AABBs.back().ID     = sentAABBs[j].id;
				AABBs.back().nameID = sentAABBs[j].atype;
				agentsAndBroadcastGeomVersions[sentAABBs[j].id] = sentAABBs[j].version;
				registerThatThisAgentIsAtThisFO(sentAABBs[j].id,i);
			}
			delete sentAABBs;
		}
	}
	DEBUG_REPORT("FO #" << this->ID << " has finished AABB reporting cycle with global size " << (AABBs.size() + agents.size()) );
	communicator->waitFor_publishAgentsAABBs();
}

void FrontOfficer::notify_publishAgentsAABBs(const int FOsID)
{
	//Dummy action due to running broadcast in cycle sending all items at once.

	//char data[] = {0}; // Use NULL instead? Would it be OK for MPI to work with?
	//communicator->sendNextFO(data,0, e_comm_tags::unblock_FO);
	//communicator->sendNextFO(data,0, e_comm_tags::send_AABB);
}


void FrontOfficer::broadcast_AABBofAgent(const ShadowAgent& ag)
{
	//Since we already do a global broadcast of all AABBs at once, we will use this method to update the AABB list, which is not done in main code (WHY?)

	DEBUG_REPORT("FO #" << this->ID << " is reporting AABB of agent ID " << ag.ID);
	int aid = ag.getID();

	const AxisAlignedBoundingBox & aabb = ag.getAABB();
	AABBs.emplace_back();
	AABBs.back().minCorner = aabb.minCorner;
	AABBs.back().maxCorner = aabb.maxCorner;
	AABBs.back().ID     = aid;
	AABBs.back().nameID = ag.getAgentTypeID();
	agentsAndBroadcastGeomVersions[aid] = ag.getGeometry().version;
	registerThatThisAgentIsAtThisFO(aid,this->ID);
}


void FrontOfficer::respond_AABBofAgent()
{
	//This method would miss important parameters and is executed in global receiving stage so it is dummy
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


void FrontOfficer::respond_Loop()
{
	// For cycle like in the Director to process D->FO messages?
	int ibuffer[DIRECTOR_RECV_MAX] = {0};
	char buffer[DIRECTOR_RECV_MAX] = {0};
	int items;
	e_comm_tags tag;
	/*static int unblock_lvl=0;*/
	//communicator->unblockNextIfSent();
	/*communicator->sendDirector(buffer,0, e_comm_tags::next_stage); //Start the director -> FO sending
	REPORT("Unblock level " << unblock_lvl << " in FO " << ID);
	if (unblock_lvl <= 0) {
		REPORT("Waiting for sync in FO " << ID);
		tag=e_comm_tags::unblock_FO;
		communicator->receiveDirectorMessage(buffer, items, tag);
		unblock_lvl=0;
	} else {
		unblock_lvl--;
	}*/
	REPORT("Running detection loop in FO " << ID);
	do {
		int instance = FO_INSTANCE_ANY;
		if ((tag = communicator->detectFOMessage()) < 0) {
			// Detect other FO messages (are there any?) and broadcasts here...
			std::this_thread::sleep_for((std::chrono::milliseconds)10);
			continue;
		}
		//tag=communicator->detectFOMessage(false);
		if (tag == e_comm_tags::ACK) {
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
			case e_comm_tags::count_AABB:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 0);
				respond_CntOfAABBs();
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
			case e_comm_tags::set_debug: //Receive and send to following FO to distribute it
				communicator->receiveFOMessage(buffer, items, instance, tag);
				setSimulationDebugRendering(buffer[0] != 0);
				respond_setRenderingDebug();
				break;
			/*case e_comm_tags::unblock_FO:
				communicator->receiveFOMessage(buffer, items, instance, tag);
				unblock_lvl++;
				break;*/
			default:
				REPORT("Unprocessed communication tag " << communicator->tagName(tag) << " on FO " << ID);
				communicator->receiveFOMessage(buffer, items, instance, tag);
				break;
		}

	} while (!this->finished); //(tag != e_comm_tags::next_stage);
	REPORT("Ending detection loop in FO " << ID << " and waiting for sync");
	//communicator->waitSync(); //waitFor_publishAgentsAABBs();
}

void FrontOfficer::waitHereUntilEveryoneIsHereToo() //Will this work without specification of stage as an argument?
{
	communicator->waitSync();
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
   //Dummy method as there is no way to pass the parameter with rendering debug flag
	DEBUG_REPORT("FO #" << this->ID << " setRendering debug to " << renderingDebug);
	char buffer[1] = { renderingDebug };
	communicator->sendNextFO(buffer, 1, e_comm_tags::set_debug);
}


void FrontOfficer::broadcast_throwException(const char* exceptionMessage)
{
	// Exceptions are be sent to stdout/stderr via REPORT_EXCEPTION and once process dies the rest of MPI does as well
	REPORT(exceptionMessage);
	exit(-1);
}


void FrontOfficer::respond_throwException()
{
	// To Director only?
	// Dummy method at least in MPI case, because exceptions can be sent to stdout/stderr and once process dies the rest of MPI does as well
}
