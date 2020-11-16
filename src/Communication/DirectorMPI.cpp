#include "DistributedCommunicator.h"
#include "../Director.h"
#include "../FrontOfficer.h"
#include <chrono>
#include <thread>

void Director::respond_getNextAvailAgentID()
{
	communicator->sendNextID(getNextAvailAgentID());
}


void Director::respond_startNewAgent()
{
	communicator->sendACKtoLastFO();  //send requested ACK back, maybe unneccessary?
}


void Director::respond_closeAgent()
{
	communicator->sendACKtoLastFO();  //send requested ACK back, maybe unneccessary?
}


void Director::respond_updateParentalLink()
{
	communicator->sendACKtoLastFO();  //send requested ACK back, maybe unneccessary?
}


void Director::notify_publishAgentsAABBs(const int FOsID)
{
	int buffer[1] = {0};
	communicator->sendFO(buffer,0,FOsID, e_comm_tags::unblock_FO);
	communicator->publishAgentsAABBs(FOsID);
	waitHereUntilEveryoneIsHereToo();
}


void Director::waitFor_publishAgentsAABBs()
{
	//This method needs to be executed so that the broadcasted messages do not stay in the queue
	DEBUG_REPORT("Director is running AABB reporting cycle");
	t_aabb * sentAABBs;
	int total_AABBs = 0;
	for (int i = 1 ; i <= FOsCount ; i++) {
		int aabb_count = communicator->cntOfAABBs(i, true);
		//In reality, following is dummy code needed to correctly distribute broadcasts through all nodes
		sentAABBs = new t_aabb[aabb_count];
		total_AABBs += aabb_count;
		communicator->receiveBroadcast(sentAABBs, aabb_count, i, e_comm_tags::send_AABB);
		delete sentAABBs;
		//End of dummy code
	}
	DEBUG_REPORT("Director has finished AABB reporting cycle with global size " << total_AABBs);
	respond_newAgentsTypes(0);
	communicator->waitFor_publishAgentsAABBs();
}


void Director::respond_AABBofAgent()
{
	//we ignore these notifications entirely
}


size_t Director::request_CntOfAABBs(const int FOsID)
{
	return communicator->cntOfAABBs(FOsID);
}


void Director::respond_newAgentsTypes(int new_dict_count)
{
	int total_cnt=0;
	DEBUG_REPORT("Director is running New Agent Type reporting cycle");
	t_hashed_str * receivedTypes;
	for (int i = 1 ; i <= FOsCount ; i++) {
		int cnt=1;
		//In reality, following is dummy code needed to correctly distribute broadcasts through all nodes
		communicator->receiveBroadcast(&new_dict_count, cnt, i, e_comm_tags::count_new_type);
		total_cnt += new_dict_count;
		receivedTypes = new t_hashed_str[new_dict_count];
		communicator->receiveBroadcast(receivedTypes, new_dict_count, i, e_comm_tags::new_type);
		delete receivedTypes;
		//End of dummy code
	}
	DEBUG_REPORT("Director is running New Agent Type reporting cycle with global size " << total_cnt);
}


void Director::notify_setDetailedDrawingMode(const int FOsID , const int agentID, const bool state)
{
	//PM: During rendering of the next frame, here the FOs should listen for director messages instead vice versa?
	communicator->setAgentsDetailedDrawingMode(FOsID, agentID, state);
}


void Director::notify_setDetailedReportingMode(const int FOsID , const int agentID, const bool state)
{
	//PM: During rendering of the next frame, here the FOs should listen for director messages instead vice versa?
	communicator->setAgentsDetailedReportingMode(FOsID, agentID, state);
}

void Director::respond_Loop()
{
	char buffer[DIRECTOR_RECV_MAX] = {0};
	int ibuffer[DIRECTOR_RECV_MAX] = {0};

	int items;
	e_comm_tags tag;
	int finished = 0;
	int waiting = 0;
	/*REPORT("Running detection loop in Director\n");
	for (int i=1; i <= FOsCount; i++) {
		communicator->sendFO(buffer,0,i, e_comm_tags::next_stage);
	}*/
	do {
		int instance = FO_INSTANCE_ANY;
		if ((tag = communicator->detectFOMessage()) < 0) {
			std::this_thread::sleep_for((std::chrono::milliseconds)10);
			continue;
		}
		//tag=communicator->detectFOMessage(false);
		if (tag == e_comm_tags::ACK) {
			std::this_thread::sleep_for((std::chrono::milliseconds)10);
			continue;
		}
		items = DIRECTOR_RECV_MAX;
		switch (tag) {
			case e_comm_tags::next_ID:
				communicator->receiveFOMessage(buffer, items, instance, tag);
				assert(items == 0);
				respond_getNextAvailAgentID();
				break;
			case e_comm_tags::new_agent:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 3);
				startNewAgent(ibuffer[0], ibuffer[1], ibuffer[2] != 0);
				respond_startNewAgent();
				break;
			case e_comm_tags::update_parent:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 2);
				startNewDaughterAgent(ibuffer[0], ibuffer[1]);
				respond_updateParentalLink();
				break;
			case e_comm_tags::close_agent:
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 2);
				closeAgent(ibuffer[0], ibuffer[1]);
				respond_closeAgent();
				break;
			case e_comm_tags::next_stage:
				communicator->receiveFOMessage(buffer, items, instance, tag);
				waiting++;
				REPORT("Waiting FOs total: " << waiting << " out of " << FOsCount << " on Director");
				if (waiting == FOsCount) {
					communicator->waitSync();
					waiting=0;
				}
				break;
			case e_comm_tags::send_AABB: //Forgotten round-robin
				communicator->receiveFOMessage(ibuffer, items, instance, tag);
				assert(items == 0);
//				finished=FOsCount;
				//Is something else called here?
				break;
/*			case e_comm_tags::unblock_FO:
				finished=FOsCount;
				communicator->receiveFOMessage(buffer, items, instance, tag);
				break;*/
			case e_comm_tags::set_debug:
				communicator->receiveFOMessage(buffer, items, instance, tag);
				break;
			case e_comm_tags::finished:
				finished++;
				REPORT("Finished FOs total: " << finished << " out of " << FOsCount << " on Director");
				break;
			default:
				REPORT("Unprocessed communication tag " << communicator->tagName(tag) << " on Director");
				communicator->receiveFOMessage(buffer, items, instance, tag);
				//PM: Mark client as done and if all are done leave the cycle
				break;
		}

	} while(finished != FOsCount);
	REPORT("Ending detection loop in Director");
	/*for (int i=1; i <= FOsCount; i++) {
		communicator->sendFO(buffer,0,i, e_comm_tags::unblock_FO);
	}*/
}

void Director::waitHereUntilEveryoneIsHereToo(/*int stage?*/) //Will this work without specification of stage as an argument?
{
	communicator->waitSync();
}


void Director::request_renderNextFrame(const int FOsID)
{
	communicator->renderNextFrame(FOsID);
}


void Director::waitFor_renderNextFrame()
{
	communicator->waitFor_renderNextFrame();
}


void Director::broadcast_setRenderingDebug(const bool setFlagToThis)
{
	char buffer[1] = { setFlagToThis };
	communicator->sendFO(buffer, 1, 1, e_comm_tags::set_debug); //Send to first FO and it will distribute it to others
}


void Director::broadcast_throwException(const char* exceptionMessage)
{
	REPORT(exceptionMessage);
	exit(-1);
}


void Director::respond_throwException()
{
   // Dummy method at least in MPI case, because exceptions can be sent to stdout/stderr and once process dies the rest of MPI does as well
}
