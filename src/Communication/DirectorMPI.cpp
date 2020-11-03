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
	communicator->publishAgentsAABBs(FOsID);
}


void Director::waitFor_publishAgentsAABBs()
{
	communicator->waitFor_publishAgentsAABBs();
}


void Director::respond_AABBofAgent()
{
	//we ignore these notifications entirely
	//
	//perhaps we would see a reason later why Direktor would need
	//to know spatial relation among all agents

	//gets : AABB +agentID +agentTypeID +geomVersion as 6x float, int, size_t, int
	//gives: nothing
}


size_t Director::request_CntOfAABBs(const int FOsID)
{
	return communicator->cntOfAABBs(FOsID);
}


void Director::respond_newAgentsTypes(int)
{
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


void Director::waitHereUntilEveryoneIsHereToo(/*int stage?*/) //Will this work without specification of stage as an argument?
{
	char buffer[DIRECTOR_RECV_MAX] = {0};
	int ibuffer[DIRECTOR_RECV_MAX] = {0};
	
	int items;
	e_comm_tags tag;
	fprintf(stderr, "Running detection loop in Director\n");
	do {
		int instance = FO_INSTANCE_ANY;
		if ((tag = communicator->detectFOMessage()) < 0) {
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
				break;
			default:
				 fprintf(stderr,"Unprocessed communication tag %i on Director\n", tag);
				 communicator->receiveFOMessage(buffer, items, instance, tag);
				 //PM: Mark client as done and if all are done leave the cycle 
				 break;
				 
		}
		
	} while(tag != e_comm_tags::next_stage);
	fprintf(stderr, "Ending detection loop in Director, will send sync\n");
	//communicator->sendSync()
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
	//SMP:
	//FO->setSimulationDebugRendering(setFlagToThis);

	//MPI world:
	//non-blocking send out to everyone
}


void Director::broadcast_throwException(const char* /* exceptionMessage */)
{
	//fprintf(stderr, ...)
	//exit(-1);

	//MPI world:
	//the same as FrontOfficer::broadcast_throwException(exceptionMessage)
}


void Director::respond_throwException()
{
	//fprintf(stderr, ...)
	//exit(-1);

	//MPI world:
	//the same as FrontOfficer::respond_throwException()
}
