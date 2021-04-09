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
	DEBUG_REPORT("FO #" << this->ID << " is running AABB reporting cycle with local size " << agents.size());
	t_aabb * sentAABBs;
	for (int i = 1 ; i <= FOsCount ; i++) {
		if (i == ID)
		{
			int aabb_count = (int)agents.size(); //getSizeOfAABBsList(); is not returning correct results! (WHY? -> Because the AABB list is empty)
			/* TBD: In future, will local AABB count in each FO always fit into integer? Otherwise we have a problem */
			int j = 0;
			communicator->sendCntOfAABBs(aabb_count, true);
			//sentAABBs = new t_aabb[aabb_count];
			sentAABBs = (t_aabb*) malloc(sizeof(t_aabb)*(aabb_count+1));
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
			free(sentAABBs);
			//delete [] sentAABBs;
		}
		else
		{
			int aabb_count = (int)communicator->cntOfAABBs(i, true);
			//sentAABBs = new t_aabb[aabb_count];
			DEBUG_REPORT("Receive " << aabb_count << " AABBs at FO#" << ID << " from FO #" << i);
			sentAABBs = (t_aabb*) malloc(sizeof(t_aabb)*(aabb_count+1));
			communicator->receiveBroadcast(sentAABBs, aabb_count, i, e_comm_tags::send_AABB);
			for (int j=0; j < aabb_count; j++) {
				AABBs.emplace_back();
				AABBs.back().minCorner = sentAABBs[j].minCorner;
				AABBs.back().maxCorner = sentAABBs[j].maxCorner;
				/*DEBUG_REPORT("FO #" << ID << ", AABB #" << j
				  << " Min: (" << sentAABBs[j].minCorner.x << "," << sentAABBs[j].minCorner.y << "," << sentAABBs[j].minCorner.z << ")"
				  << " Max: (" << sentAABBs[j].maxCorner.x << "," << sentAABBs[j].maxCorner.y << "," << sentAABBs[j].maxCorner.z << ")"
				);*/
				AABBs.back().ID     = sentAABBs[j].id;
				AABBs.back().nameID = sentAABBs[j].atype;
				agentsAndBroadcastGeomVersions[sentAABBs[j].id] = sentAABBs[j].version;
				/*DEBUG_REPORT("FO #" << ID << ", Agent #" << j << " ID: " << sentAABBs[j].id
				  << " Name ID: " << sentAABBs[j].atype <<  ", Geometry version " <<sentAABBs[j].version
				);*/
				registerThatThisAgentIsAtThisFO(sentAABBs[j].id,i);
			}
			free(sentAABBs);
//			delete [] sentAABBs;
		}
	}
	communicator->waitFor_publishAgentsAABBs();
	DEBUG_REPORT("FO #" << this->ID << " has finished AABB reporting cycle with global size " << (AABBs.size() + agents.size()) );
	broadcast_newAgentsTypes(); //Force-call it here?
}

void FrontOfficer::notify_publishAgentsAABBs(const int /*FOsID*/)
{
	//Dummy action due to running broadcast in cycle sending all items at once.
}


void FrontOfficer::broadcast_AABBofAgent(const ShadowAgent& ag)
{
	//Since we already do a global broadcast of all AABBs at once, we will use this method to update the AABB list, which is not done in main code (WHY?)

	DEBUG_REPORT("FO #" << this->ID << " is adding AABB of shadow agent ID " << ag.ID);
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
#ifdef DEBUG
	size_t sendBackMyCount = getSizeOfAABBsList();
	REPORT("FO #" << ID << " Count: " << sendBackMyCount);
	//communicator->sendCntOfAABBs(sendBackMyCount);
#endif
}


void FrontOfficer::broadcast_newAgentsTypes()
{
	int new_dict_count = (int)agentsTypesDictionary.howManyShouldBeBroadcast()/*Integer limit !!!*/, total_cnt=0;
	DEBUG_REPORT("FO #" << this->ID << " is running New Agent Type reporting cycle with local size " << new_dict_count);
	t_hashed_str * sentTypes;
/*	char * sentTypes;*/
	for (int i = 1 ; i <= FOsCount ; i++) {
		if (i == ID)
		{
			total_cnt += new_dict_count = (int)agentsTypesDictionary.howManyShouldBeBroadcast();
			int j = 0;
			DEBUG_REPORT("New dictionary count From FO#" << i << ": " << new_dict_count);
			communicator->sendBroadcast(&new_dict_count, 1, i, e_comm_tags::count_new_type);
			//sentTypes = new t_hashed_str[new_dict_count];
			sentTypes = (t_hashed_str*)calloc(sizeof(t_hashed_str),new_dict_count+1);
/*			new_dict_count *= StringsImprintSize;
			sentTypes = new char[new_dict_count];*/
			for (const auto& dItem : agentsTypesDictionary.theseShouldBeBroadcast())
			{
				sentTypes[j].hash = dItem.first;
				hashedString::printIntoBuffer(dItem.second, sentTypes[j].value,StringsImprintSize);
				//DEBUG_REPORT("Sending Type " <<  sentTypes[j].value << " with hash " << dItem.first << " from FO#" << i);
				/*hashedString::printIntoBuffer(dItem.second, sentTypes+j*StringsImprintSize,StringsImprintSize);*/
				j++;
			}
			communicator->sendBroadcast(sentTypes, new_dict_count, i, e_comm_tags::new_type);
			free(sentTypes);
//			delete [] sentTypes;
		}
		else
		{
			int cnt=1;
			communicator->receiveBroadcast(&new_dict_count, cnt, i, e_comm_tags::count_new_type);
			assert(cnt==1);
			total_cnt += new_dict_count;
			//sentTypes = new t_hashed_str[new_dict_count];
			sentTypes = (t_hashed_str*)calloc(sizeof(t_hashed_str),new_dict_count+1);
/*			new_dict_count *= StringsImprintSize;
			sentTypes = new char[new_dict_count];*/
			communicator->receiveBroadcast(sentTypes, new_dict_count, i, e_comm_tags::new_type);
			for (int j=0; j < new_dict_count; j++)
			{
				/*hashedString * hs = new hashedString(sentTypes+j*StringsImprintSize);
				agentsTypesDictionary.enlistTheIncomingItem(hs->getHash(), hs->getString());*/
				agentsTypesDictionary.enlistTheIncomingItem(sentTypes[j].hash, sentTypes[j].value);
				DEBUG_REPORT("Receiving Type " <<  sentTypes[j].value << " with hash " << sentTypes[j].hash << " at FO#" << ID);
			}
			free(sentTypes);
//			delete [] sentTypes;
		}
	}
	agentsTypesDictionary.markAllWasBroadcast();
	//DEBUG_REPORT("Checking hash for nucleus 2015 @ 4,2:" << agentsTypesDictionary.translateIdToString(5627021567199719035l));
#ifdef DISTRIBUTED_DEBUG
	agentsTypesDictionary.printKnownDictionary();
#endif /*DISTRIBUTED_DEBUG*/
	communicator->waitFor_publishAgentsAABBs();
	DEBUG_REPORT("FO #" << this->ID << " has finished New Agent Type reporting cycle with global size " << total_cnt);
}


void FrontOfficer::respond_newAgentsTypes(int /*noOfIncomingNewAgentTypes*/)
{
	//This method would miss important parameters and is executed in global receiving stage so it is dummy
}


ShadowAgent* FrontOfficer::request_ShadowAgentCopy(const int agentID, const int FOsID)
{
	size_t param_buff[4] =  {(size_t)agentID,0,0,0};
	int cnt = 4; //sizeof(param_buff) / sizeof(long*);
	int fo_back = FOsID;
	int items;
	e_comm_tags tag = e_comm_tags::shadow_copy;

	communicator->sendFO(param_buff, 1, FOsID, e_comm_tags::get_shadow_copy);
	communicator->receiveFOMessage(param_buff, cnt, fo_back, tag);
	DEBUG_REPORT("Received shadow copy info at FO #"  << ID << ": ID=" << param_buff[0] << ", Type=" << param_buff[2] << ", Geom Type=" << param_buff[3]);
	items= (int)param_buff[1];
	char * data_buff = (char *) malloc(items);

	tag=e_comm_tags::shadow_copy_data;
	communicator->receiveFOMessage(data_buff, items, fo_back, tag);

	int         gotThisAgentID   = (int) param_buff[0];
	std::string gotThisAgentType = agentsTypesDictionary.translateIdToString((size_t)param_buff[2]);
	Geometry*   gotThisGeom      = Geometry::createAndDeserializeFrom((int)param_buff[3], data_buff);
	free(data_buff);

	return new ShadowAgent(*gotThisGeom, gotThisAgentID,gotThisAgentType);
}


void FrontOfficer::respond_ShadowAgentCopy(const int agentID)
{
#ifdef DEBUG
	if (agents.find(agentID) == agents.end())
		throw ERROR_REPORT("Cannot provide ShadowAgent for agent ID " << agentID);
#endif

	int foID = communicator->getLastFOID();

	const AbstractAgent& aaRef = *(agents[agentID]);
	const Geometry& sendBackGeom = aaRef.getGeometry(); // Correct geometry -> simplified
	int sendBackAgentID   = aaRef.getID();
	const long geom_size = sendBackGeom.getSizeInBytes();
	const size_t sendBackAgentType = aaRef.getAgentTypeID();
	const int geom_type = (int) sendBackGeom.shapeForm;

	size_t param_buff[4] =  {(size_t)sendBackAgentID, (size_t)geom_size, sendBackAgentType, (size_t)geom_type};
	int cnt = 4; //sizeof(param_buff) / sizeof(long*);

	DEBUG_REPORT("Sent shadow copy info at FO #"  << ID << ": ID=" << param_buff[0] << ", Type=" << param_buff[2] << ", Geom Type=" << param_buff[3]);
	communicator->sendFO(param_buff, cnt, foID, e_comm_tags::shadow_copy);

	std::unique_ptr<char[]> buffer_to_send( new char[geom_size] );
	//char buffer_to_send [geom_size];
	sendBackGeom.serializeTo(buffer_to_send.get());

	communicator->sendFO(buffer_to_send.get(), (int)geom_size, foID, e_comm_tags::shadow_copy_data);
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
	int ibuffer[DIRECTOR_RECV_MAX] /*= {0}*/;
	char buffer[DIRECTOR_RECV_MAX] /*= {0}*/;
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
		items = DIRECTOR_RECV_MAX;
		int instance = FO_INSTANCE_ANY;
		if ((tag = communicator->detectFOMessage(false/*we are runnung synchronously in the thread*/)) < 0) {
			// Detect other FO messages (are there any?) and broadcasts here...
			std::this_thread::sleep_for((std::chrono::milliseconds)10);
			continue;
		}
		//tag=communicator->detectFOMessage(false);
		if (communicator->isFinished()) { break; }
		if (tag == e_comm_tags::ACK || tag == e_comm_tags::unblock_FO) {
			REPORT("ACK/Unblock on Director Communicator on FO #" << ID);
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
			case e_comm_tags::get_count_AABB:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 0);
				respond_CntOfAABBs();
				break;
			case e_comm_tags::get_shadow_copy:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 1);
				respond_ShadowAgentCopy(ibuffer[0]);
				break;
			case e_comm_tags::send_AABB:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 0);
				//Is something else called here?
				notify_publishAgentsAABBs(nextFOsID);
				break;
			//Chyba je tady!
			case e_comm_tags::render_frame:
				communicator->receiveFOMessage(buffer, items, instance, tag);
				assert(items == 0);
				break;
			/*case e_comm_tags::next_stage:
				communicator->receiveFOMessage(buffer, items, instance, tag);
				communicator->waitSync(e_comm_tags::next_stage);
				break;*/
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

	} while (!this->finished && !communicator->isFinished()); //(tag != e_comm_tags::next_stage);
	REPORT("Ending detection loop in FO " << ID << " and waiting for sync");
	//communicator->waitSync(); //waitFor_publishAgentsAABBs();
}

void FrontOfficer::waitHereUntilEveryoneIsHereToo() //Will this work without specification of stage as an argument?
{
	DEBUG_REPORT("Wait Here Until Everyone Is Here Too FO #" << ID);
	communicator->waitSync();
}


void FrontOfficer::waitFor_renderNextFrame(const int FOsID)
{
	SceneControls& sc = scenario.params;
	if (! (sc.imagesSaving_isEnabledForImgMask() || sc.imagesSaving_isEnabledForImgPhantom()
													|| sc.imagesSaving_isEnabledForImgOptics())) {
		return; /*Nothing to exchange*/
	}

	size_t maskPixelLength=0;
	int maskZSize= 0;
	int maskXYSize= 0;

	if (sc.imagesSaving_isEnabledForImgMask()) {
		maskPixelLength=sc.imgMask.GetImageSize();
		maskZSize=(int)sc.imgMask.GetSizeZ();
	} else if (sc.imagesSaving_isEnabledForImgPhantom()) {
		maskPixelLength=sc.imgPhantom.GetImageSize();
		maskZSize=(int)sc.imgPhantom.GetSizeZ();
	} else {
		maskPixelLength=sc.imgOptics.GetImageSize();
		maskZSize=(int)sc.imgOptics.GetSizeZ();
	}
	maskXYSize=(int)(maskPixelLength/maskZSize);

	communicator->waitFor_renderNextFrame();
	unsigned short * maskPixelBuffer = NULL;
	float * phantomBuffer  = NULL;
	float * opticsBuffer  = NULL;

	if (sc.imagesSaving_isEnabledForImgMask()) { maskPixelBuffer = sc.imgMask.GetFirstVoxelAddr();}
	if (sc.imagesSaving_isEnabledForImgPhantom()) { phantomBuffer = sc.imgPhantom.GetFirstVoxelAddr();}
	if (sc.imagesSaving_isEnabledForImgOptics()) { opticsBuffer = sc.imgOptics.GetFirstVoxelAddr();}

	DEBUG_REPORT("Request image merging from FO #" << ID << " to FO #" << FOsID);
	DEBUG_REPORT("Mask enabled: " << sc.imagesSaving_isEnabledForImgMask()
	               << ", phantom enabled: " << sc.imagesSaving_isEnabledForImgPhantom()
	               << ", optics enabled: " << sc.imagesSaving_isEnabledForImgOptics()
	);
	communicator->mergeImages(FOsID, maskXYSize, maskZSize, maskPixelBuffer, phantomBuffer, opticsBuffer);
	DEBUG_REPORT("Image merging done on FO #" << ID);
}


void FrontOfficer::request_renderNextFrame(const int FOsID)
{
	communicator->renderNextFrame(FOsID);
}


void FrontOfficer::respond_setRenderingDebug()
{
   //Dummy method as there is no way to pass the parameter with rendering debug flag
	DEBUG_REPORT("FO #" << this->ID << " setRendering debug to " << renderingDebug);
	char buffer[1] = { renderingDebug };
	communicator->sendNextFO(buffer, 1, e_comm_tags::set_debug);
}

void FrontOfficer::close_communication()
{
	communicator->close();
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
