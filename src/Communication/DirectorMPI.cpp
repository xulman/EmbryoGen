#include "../Director.hpp"
#include "../FrontOfficer.hpp"
#include "DistributedCommunicator.hpp"
#include <chrono>
#include <thread>
#include <cassert>

void Director::respond_getNextAvailAgentID() {
	communicator->sendNextID(getNextAvailAgentID());
}

void Director::respond_startNewAgent(int FO) {
	communicator->sendACKtoFO(
	    FO); // send requested ACK back, maybe unneccessary?
}

void Director::respond_closeAgent(int FO) {
	communicator->sendACKtoFO(
	    FO); // send requested ACK back, maybe unneccessary?
}

void Director::respond_updateParentalLink(int FO) {
	communicator->sendACKtoFO(
	    FO); // send requested ACK back, maybe unneccessary?
}

void Director::notify_publishAgentsAABBs(const int FOsID) {
	// int buffer[1] = {0};
	// communicator->sendFO(buffer,0,FOsID, e_comm_tags::unblock_FO);
	communicator->publishAgentsAABBs(FOsID);
}

void Director::waitFor_publishAgentsAABBs() {
	// This method needs to be executed so that the broadcasted messages do not
	// stay in the queue
	report::debugMessage(
	    fmt::format("Director is running AABB reporting cycle"));
	t_aabb* sentAABBs;
	int total_AABBs = 0;
	for (int i = 1; i <= FOsCount; i++) {
		int aabb_count = (int)communicator->cntOfAABBs(i, true);
		// In reality, following is dummy code needed to correctly distribute
		// broadcasts through all nodes sentAABBs = new t_aabb[aabb_count];
		sentAABBs = (t_aabb*)malloc(sizeof(t_aabb) * (aabb_count + 1));
		total_AABBs += aabb_count;
		communicator->receiveBroadcast(sentAABBs, aabb_count, i,
		                               e_comm_tags::send_AABB);
		// delete [] sentAABBs;
		free(sentAABBs);
		// End of dummy code
	}
	communicator->waitFor_publishAgentsAABBs();
	report::debugMessage(fmt::format(
	    "Director has finished AABB reporting cycle with global size {}",
	    total_AABBs));
	respond_newAgentsTypes(0);
}

void Director::respond_AABBofAgent() {
	// we ignore these notifications entirely
}

size_t Director::request_CntOfAABBs(const int FOsID) {
	return communicator->cntOfAABBs(FOsID);
}

void Director::respond_newAgentsTypes(int /*new_dict_count*/) {
	int total_cnt = 0;
	t_hashed_str* receivedTypes;
	//	char * receivedTypes;
	int partial_dict_count;
	report::debugMessage(fmt::format("Director is running New Agent Type "
	                                 "reporting cycle with global size {}",
	                                 total_cnt));
	for (int i = 1; i <= FOsCount; i++) {
		int cnt = 1;
		// In reality, following is dummy code needed to correctly distribute
		// broadcasts through all nodes
		communicator->receiveBroadcast(&partial_dict_count, cnt, i,
		                               e_comm_tags::count_new_type);
		total_cnt += partial_dict_count;
		report::debugMessage(
		    fmt::format("New dictionary count received on Director from FO#{} "
		                "(out of {}): {} (out of {})",
		                i, FOsCount, partial_dict_count, total_cnt));
		receivedTypes =
		    (t_hashed_str*)calloc(sizeof(t_hashed_str), partial_dict_count + 1);
		/*partial_dict_count *= StringsImprintSize;
		receivedTypes = new char[partial_dict_count];*/
		communicator->receiveBroadcast(receivedTypes, partial_dict_count, i,
		                               e_comm_tags::new_type);
		free(receivedTypes);
		// End of dummy code
	}
	communicator->waitFor_publishAgentsAABBs();
	report::debugMessage(fmt::format("Director has finished New Agent Type "
	                                 "reporting cycle with global size {}",
	                                 total_cnt));
}

void Director::notify_setDetailedDrawingMode(const int FOsID,
                                             const int agentID,
                                             const bool state) {
	// PM: During rendering of the next frame, here the FOs should listen for
	// director messages instead vice versa?
	communicator->setAgentsDetailedDrawingMode(FOsID, agentID, state);
}

void Director::notify_setDetailedReportingMode(const int FOsID,
                                               const int agentID,
                                               const bool state) {
	// PM: During rendering of the next frame, here the FOs should listen for
	// director messages instead vice versa?
	communicator->setAgentsDetailedReportingMode(FOsID, agentID, state);
}

void Director::respond_Loop() {
	char buffer[DIRECTOR_RECV_MAX] /*= {0}*/;
	int ibuffer[DIRECTOR_RECV_MAX] /*= {0}*/;

	int items;
	e_comm_tags tag;
	int finished = 0;
	/*REPORT("Running detection loop in Director\n");
	for (int i=1; i <= FOsCount; i++) {
	    communicator->sendFO(buffer,0,i, e_comm_tags::next_stage);
	}*/
	do {
		int instance = FO_INSTANCE_ANY;
		/*if ((tag = communicator->detectFOMessage()) < 0) {
		    std::this_thread::sleep_for((std::chrono::milliseconds)10);
		    continue;
		}*/
		tag = communicator->detectFOMessage(false);
		if (communicator->isFinished()) {
			break;
		}
		if (tag == e_comm_tags::ACK) {
			report::message(
			    fmt::format("ACK on Director Communicator on Director"));
			std::this_thread::sleep_for((std::chrono::milliseconds)10);
			continue;
		}
		items = DIRECTOR_RECV_MAX;
		switch (tag) {
		case e_comm_tags::get_next_ID:
			communicator->receiveFOMessage(buffer, items, instance, tag);
			assert(items == 0);
			respond_getNextAvailAgentID();
			break;
		case e_comm_tags::new_agent:
			communicator->receiveFOMessage(ibuffer, items, instance, tag);
			assert(items == 3);
			startNewAgent(ibuffer[0], ibuffer[1], ibuffer[2] != 0);
			respond_startNewAgent(instance);
			break;
		case e_comm_tags::update_parent:
			communicator->receiveFOMessage(ibuffer, items, instance, tag);
			assert(items == 2);
			startNewDaughterAgent(ibuffer[0], ibuffer[1]);
			respond_updateParentalLink(instance);
			break;
		case e_comm_tags::close_agent:
			communicator->receiveFOMessage(ibuffer, items, instance, tag);
			assert(items == 2);
			closeAgent(ibuffer[0], ibuffer[1]);
			respond_closeAgent(instance);
			break;
		case e_comm_tags::send_AABB: // Forgotten round-robin
			communicator->receiveFOMessage(ibuffer, items, instance, tag);
			assert(items == 0);
			//				finished=FOsCount;
			// Is something else called here?
			break;
		case e_comm_tags::render_frame:
			communicator->receiveFOMessage(buffer, items, instance, tag);
			assert(items == 0);
			break;
		case e_comm_tags::set_debug:
			communicator->receiveFOMessage(buffer, items, instance, tag);
			break;
		case e_comm_tags::finished:
			finished++;
			report::message(
			    fmt::format("Finished FOs total: {} out of {} on Director",
			                finished, FOsCount));
			break;
		default:
			report::message(
			    fmt::format("Unprocessed communication tag {} on Director",
			                communicator->tagName(tag)));
			communicator->receiveFOMessage(buffer, items, instance, tag);
			// PM: Mark client as done and if all are done leave the cycle
			break;
		}

	} while (finished != FOsCount && !communicator->isFinished());
	report::message(fmt::format("Ending detection loop in Director"));
	/*for (int i=1; i <= FOsCount; i++) {
	    communicator->sendFO(buffer,0,i, e_comm_tags::unblock_FO);
	}*/
}

void Director::waitHereUntilEveryoneIsHereToo(
    /*int stage?*/) // Will this work without specification of stage as an
                    // argument?
{
	report::debugMessage(
	    fmt::format("Wait Here Until Everyone Is Here Too Director"));
	communicator->waitSync();
}

void Director::request_renderNextFrame(const int FOsID) {
	report::message(fmt::format("Request render next frame on Director"));
	communicator->renderNextFrame(FOsID);
	communicator->waitFor_renderNextFrame();
}

void Director::waitFor_renderNextFrame(const int FOsID) {
	SceneControls& sc = *scenario.params;
	size_t maskPixelLength = 0;
	int maskZSize = 0;
	int maskXYSize = 0;

	if (sc.imagesSaving_isEnabledForImgMask()) {
		maskPixelLength = sc.imgMask.GetImageSize();
		maskZSize = (int)sc.imgMask.GetSizeZ();
	} else if (sc.imagesSaving_isEnabledForImgPhantom()) {
		maskPixelLength = sc.imgPhantom.GetImageSize();
		maskZSize = (int)sc.imgPhantom.GetSizeZ();
	} else {
		maskPixelLength = sc.imgOptics.GetImageSize();
		maskZSize = (int)sc.imgOptics.GetSizeZ();
	}
	maskXYSize = (int)(maskPixelLength / maskZSize);

	// request_renderNextFrame(1);
	unsigned short* maskPixelBuffer = NULL;
	float* phantomBuffer = NULL;
	float* opticsBuffer = NULL;

	if (sc.imagesSaving_isEnabledForImgMask()) {
		maskPixelBuffer = sc.imgMask.GetFirstVoxelAddr();
	}
	if (sc.imagesSaving_isEnabledForImgPhantom()) {
		phantomBuffer = sc.imgPhantom.GetFirstVoxelAddr();
	}
	if (sc.imagesSaving_isEnabledForImgOptics()) {
		opticsBuffer = sc.imgOptics.GetFirstVoxelAddr();
	}

	report::debugMessage(
	    fmt::format("Request image merging from Director to FO #{}", FOsID));
	report::debugMessage(
	    fmt::format("Mask enabled: {}, phantom enabled: {}, optics enabled: {}",
	                sc.imagesSaving_isEnabledForImgMask(),
	                sc.imagesSaving_isEnabledForImgPhantom(),
	                sc.imagesSaving_isEnabledForImgOptics()));
	communicator->mergeImages(FOsID, maskXYSize, maskZSize, maskPixelBuffer,
	                          phantomBuffer, opticsBuffer);
	report::debugMessage(fmt::format("Image merging done on Director"));
}

void Director::broadcast_setRenderingDebug(const bool setFlagToThis) {
	char buffer[1] = {setFlagToThis};
	communicator->sendFO(
	    buffer, 1, 1, e_comm_tags::set_debug); // Send to first FO and it will
	                                           // distribute it to others
}

void Director::broadcast_throwException(const std::string& exceptionMessage) {
	report::message(exceptionMessage);
	exit(-1);
}

void Director::close_communication() { communicator->close(); }

void Director::respond_throwException() {
	// Dummy method at least in MPI case, because exceptions can be sent to
	// stdout/stderr and once process dies the rest of MPI does as well
}
