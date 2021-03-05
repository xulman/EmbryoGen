#include "DistributedCommunicator.h"
#include <chrono>
#include <thread>

// ID v batchi po 1000 a pak zpracovat? Ale co newAgent a ParentalLink, to stejně nepomůže. -> Do budoucna, zatím ne.


#ifdef DISTRIBUTED
char MPI_Communicator::no_message[1] = {0};

MPI_Communicator::~MPI_Communicator() {
// Did not work correctly:	MPI_Finalize();
}

int MPI_Communicator::init(int argc, char **argv) {
	int cshift = 1, icnt;
	int out_model=0;
	//const int director_id [1] = {0};
	//int state = MPI_Init(&argc, &argv);
	int state = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &out_model);

	if (state != MPI_SUCCESS) {  return state; }

	if (out_model != MPI_THREAD_MULTIPLE) {
        REPORT("Could not initialize MPI with thread support");
        MPI_Abort(MPI_COMM_WORLD,1);
		exit(-1);
	}

	MPI_Comm_size(MPI_COMM_WORLD, &instances);
	MPI_Comm_rank(MPI_COMM_WORLD, &instance_ID);

    const int v3d_count = 3;
    int v3d_blocks[] = {1,1,1};
    MPI_Aint v3d_displ[] = {offsetof(Vector3d<float>, x), offsetof(Vector3d<float>, y), offsetof(Vector3d<float>, z) };
    MPI_Datatype v3d_types[] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};

    MPI_Type_create_struct(v3d_count, v3d_blocks, v3d_displ, v3d_types, &MPI_VECTOR3D);
    MPI_Type_commit(&MPI_VECTOR3D);

    const int aabb_count = 5;
    int aabb_blocks[] = {1, 1, 1, 1, 1};
    MPI_Aint aabb_displ[] = {offsetof(t_aabb, minCorner), offsetof(t_aabb, maxCorner), offsetof(t_aabb, id), offsetof(t_aabb, version), offsetof(t_aabb, atype) };
    MPI_Datatype aabb_types[] = {MPI_VECTOR3D, MPI_VECTOR3D, MPI_INT, MPI_INT, MPI_UINT64_T};

    MPI_Type_create_struct(aabb_count, aabb_blocks, aabb_displ, aabb_types, &MPI_AABB);
    state = MPI_Type_commit(&MPI_AABB);

    const int agentt_count = 2;
    int agentt_blocks[] = {1, StringsImprintSize};
    MPI_Aint agentt_displ[] = {offsetof(t_hashed_str, hash), offsetof(t_hashed_str, value)};
    MPI_Datatype agentt_types[] = {MPI_UNSIGNED_LONG_LONG, MPI_CHAR};
    MPI_Type_create_struct(agentt_count, agentt_blocks, agentt_displ, agentt_types, &MPI_AGENTTYPE);
    MPI_Type_commit(&MPI_AGENTTYPE);


	//director_comm = MPI_COMM_WORLD;
	MPI_Comm_dup(MPI_COMM_WORLD, &director_comm);
	MPI_Comm_set_name(director_comm, "Director_comm");

	//MPI_Comm_dup(MPI_COMM_WORLD, &token_comm);
	//MPI_Comm_split(MPI_COMM_WORLD, 1, instance_ID, &token_comm);

	/*
	//Does not work correctly why?
	MPI_Group FO_group;
	MPI_Group world_group;

	MPI_Comm_group(MPI_COMM_WORLD, &world_group);
	MPI_Group_excl(world_group, 1, director_id, &FO_group);
	MPI_Comm_create(MPI_COMM_WORLD, FO_group, &aabb_comm);

	//Probably does not work correctly either?
	//MPI_Comm_split(MPI_COMM_WORLD, (instance_ID != *director_id), instance_ID, &aabb_comm);

	// For both cases, we would have to renumber FOs from 0 to FOCnt-1 !
	*/

	MPI_Comm_dup(MPI_COMM_WORLD, &aabb_comm);
	MPI_Comm_dup(/*MPI_COMM_WORLD*/aabb_comm, &type_comm); //TBD: Or aabb_comm?
	MPI_Comm_dup(MPI_COMM_WORLD, &image_comm);
	MPI_Comm_dup(MPI_COMM_WORLD, &barrier_comm);
	MPI_Comm_dup(MPI_COMM_WORLD, &id_comm);

	MPI_Comm_set_name(aabb_comm, "AABB_comm");
	MPI_Comm_set_name(type_comm, "AgentType_comm");
	MPI_Comm_set_name(image_comm, "Image_comm");
	MPI_Comm_set_name(id_comm, "ID sender");
	MPI_Comm_set_name(barrier_comm, "Barrier_comm");

	/* //Code for shifting agent IDs if used locally, unused now
	for (cshift=1, icnt=instances; icnt != 0 ; icnt = icnt	 >> 1) {
		cshift++;
	}
	shift = sizeof(int)*8 - cshift;
	DEBUG_REPORT("Shift agent ID by " << shift << " bits\n");
	*/

    MPI_Get_processor_name(processor_name, &name_len);
	return state;
}

bool MPI_Communicator::detectMPIMessage(MPI_Comm comm, int peer, e_comm_tags & tag, bool async) {
	int flag;
	MPI_Status status;
	int MPI_tag = (tag == e_comm_tags::unspecified)?MPI_ANY_TAG:tag;

	int state;

	if (async) {
		state = MPI_Iprobe(peer, MPI_tag, comm, &flag, &status);
	} else {
		state = MPI_Probe(peer, MPI_tag, comm, &status);
		flag=1;
	}

	if (state != MPI_SUCCESS) { return false; }
	if (flag) {
		//debugMPIComm("Poll", comm, flag, peer, tag);
		tag = (e_comm_tags) status.MPI_TAG;
	}

	return flag; // Message available
}

bool MPI_Communicator::receiveAndProcessDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag) {
//	MPI_Status status;
	bool result;
	int state;

//	do { --
	result = detectMPIMessage(director_comm, DIRECTOR_ID, tag, false);
	if (result) {
		return receiveDirectorMessage(buffer, recv_size, tag);
	}
//	std::this_thread::sleep_for((std::chrono::milliseconds)1); // Check
//	} while (!result);
	return false;
}

bool MPI_Communicator::receiveDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag, bool async) {
	MPI_Status status;
	bool result;
	int state;
	static int did=DIRECTOR_ID;

	//MPI_Comm comm = (tag == e_comm_tags::ACK)?MPI_COMM_WORLD:director_comm;
	MPI_Comm comm = tagCommMap(tag, director_comm);

	state = receiveMPIMessage(comm, buffer, recv_size, tagMap(tag), &status, did, tag);
	if (state == MPI_SUCCESS) {
		tag = (e_comm_tags) status.MPI_TAG;
		return true;
	}
	return false;
}

int MPI_Communicator::receiveMPIMessage(MPI_Comm comm, void * data,  int & items, MPI_Datatype datatype, MPI_Status *status, int & peer, e_comm_tags tag) {
	debugMPIComm("Ask to receive", comm, items, peer, tag);
	MPI_Status local_status;
	if (status == MPI_STATUSES_IGNORE) { status = &local_status; }

	int state = MPI_Recv(data, items, datatype, peer, tag, comm, status);
	if (state == MPI_SUCCESS) {
		state = MPI_Get_count(status, datatype, &items);
		peer = status->MPI_SOURCE;
	}
	debugMPIComm("Received", comm, items, peer, tag);
	return state;
}


e_comm_tags MPI_Communicator::detectFOMessage(bool async) {
	e_comm_tags tag = e_comm_tags::unspecified;
	if (detectMPIMessage(director_comm, MPI_ANY_SOURCE, tag, async)) {
		return tag;
	} else {
		return e_comm_tags::unspecified;
	}
}

bool MPI_Communicator::receiveFOMessage(void * buffer, int &recv_size, int & instance_ID, e_comm_tags &tag) {
	MPI_Status status;
	bool result;
	int state;

	//MPI_Comm comm = (tag == e_comm_tags::ACK)?MPI_COMM_WORLD:director_comm;
	MPI_Comm comm = tagCommMap(tag, director_comm);
	if (instance_ID == 0) { instance_ID = MPI_ANY_SOURCE;}

	state = receiveMPIMessage(comm, buffer, recv_size, tagMap(tag), &status, instance_ID, tag);
	if (state == MPI_SUCCESS) {
		tag = (e_comm_tags) status.MPI_TAG;
		lastFOID = status.MPI_SOURCE;
		return true;
	}
	return false;
}


int MPI_Communicator::getNextAvailAgentID()
{
	//return Direktor->getNextAvailAgentID();
	//blocks and waits until it gets int back from the Director
	e_comm_tags tag = e_comm_tags::next_ID;
	int id_cnt = 1;
	int buffer [] = {0};
	sendDirector(buffer, 0, e_comm_tags::get_next_ID);
	receiveDirectorMessage(buffer, id_cnt, tag);
	DEBUG_REPORT("From FO " << instance_ID << " to Director: get new agent ID=" << buffer[0]);
	return buffer[0];
}

void MPI_Communicator::startNewAgent(const int newAgentID, const int associatedFO, const bool wantsToAppearInCTCtracksTXTfile)
{
	int buffer [] = {newAgentID, associatedFO, wantsToAppearInCTCtracksTXTfile};
	DEBUG_REPORT("From FO " << associatedFO << " to Director: start new agent ID=" << newAgentID << ((wantsToAppearInCTCtracksTXTfile)?" (CTC)":"") );
	sendDirector(buffer, sizeof(buffer)/sizeof(int), e_comm_tags::new_agent);
	receiveDirectorACK();
}

void MPI_Communicator::closeAgent(const int agentID, const int associatedFO)
{
	int buffer [] = {agentID, associatedFO};
	sendDirector(buffer, sizeof(buffer)/sizeof(int), e_comm_tags::close_agent);
	receiveDirectorACK();
	DEBUG_REPORT("From FO " << associatedFO << " to Director: close agent ID=" << agentID );
}

void MPI_Communicator::startNewDaughterAgent(const int childID, const int parentID)
{
	int buffer [] = {childID, parentID};
	sendDirector(buffer, sizeof(buffer)/sizeof(int), e_comm_tags::update_parent);
	receiveDirectorACK();
}

void MPI_Communicator::setAgentsDetailedDrawingMode(int FO, int agentID, bool state)
{
	int buffer [] = {agentID, state};
	sendFO(buffer, sizeof(buffer)/sizeof(int), FO, e_comm_tags::set_detailed_drawing);
	receiveFOACK(FO);
}

void MPI_Communicator::setAgentsDetailedReportingMode(int FO, int agentID, bool state)
{
	int buffer [] = {agentID, state};
	sendFO(buffer, sizeof(buffer)/sizeof(int), FO, e_comm_tags::set_detailed_reporting);
	receiveFOACK(FO);
}

void MPI_Communicator::publishAgentsAABBs(int FO)
{
	int buffer [] = {0};
	sendFO(buffer, 0, FO, e_comm_tags::send_AABB); //FO!
	//receiveFOACK(FO);
}

void MPI_Communicator::renderNextFrame(int FO)
{
	int buffer [] = {0};
	sendFO(buffer, 0, FO, e_comm_tags::render_frame);
//	receiveFOACK(FO);
}

size_t MPI_Communicator::receiveRenderedFrame(int fromFO, int slice_size, int slices) {
	/* Unused right now due to mergeImages method*/
}

void MPI_Communicator::mergeImages(int FO, int slice_size, int slices, unsigned short * maskPixelBuffer, float * phantomBuffer, float * opticsBuffer) {
	unsigned short * mask_add  = (unsigned short *)malloc((slice_size+1) * sizeof(unsigned short));
	float * phantom_add = (float *)malloc((slice_size+1) * sizeof(float));
	float * optics_add = (float *) malloc(slice_size * sizeof(float));
	//unsigned short MAX_LIGHT = 65536;
	e_comm_tags rmask = e_comm_tags::mask_data;
	e_comm_tags rimg = e_comm_tags::float_image_data;

	DEBUG_REPORT("From #" << instance_ID << " to #" << FO << " merge images plane size " << slice_size << " slices " << slices);
	for (int slice = 0 ; slice < slices; slice++) {
		int received_slice_size = (int) slice_size;
		int received_from = MPI_ANY_SOURCE;
		unsigned short * mask_start = maskPixelBuffer+slice*sizeof(unsigned short);
		float * phantom_start = phantomBuffer+slice*sizeof(float);
		float * optics_start = opticsBuffer+slice*sizeof(float);
		if (instance_ID) { // Front officer - receives and send later
			REPORT("Receive at FO #" << instance_ID << " slice " << slice);
			if (maskPixelBuffer) { receiveMPIMessage(image_comm, mask_add, received_slice_size, tagMap(rmask), MPI_STATUSES_IGNORE, received_from, rmask); }
			if (phantomBuffer) { receiveMPIMessage(image_comm, phantom_add, received_slice_size, tagMap(rimg), MPI_STATUSES_IGNORE, received_from, rimg);  }
			if (opticsBuffer) { receiveMPIMessage(image_comm, optics_add, received_slice_size, tagMap(rimg), MPI_STATUSES_IGNORE, received_from, rimg); }
//			assert(received_slice_size == slice_size && received_from == instance_ID - 1);
			for (int idx=0; idx < slice_size; idx++) {
				if (maskPixelBuffer){ mask_start[idx] += mask_add[idx]; }
				if (phantomBuffer) 	{ phantom_start[idx] += phantom_add[idx]; }
				if (opticsBuffer) 	{ optics_start[idx] += optics_add[idx]; } //Probably Not correct, should be mean/median later?
			}
		}

		DEBUG_REPORT("Send from #" << instance_ID << " to #" << FO << " slice " << slice);
		//TBD Check if Director's image is zeroed
		if (maskPixelBuffer) { sendMPIMessage(image_comm, mask_start, slice_size, tagMap(e_comm_tags::mask_data), FO , e_comm_tags::mask_data); }
		if (phantomBuffer) { sendMPIMessage(image_comm, phantom_start, slice_size, tagMap(e_comm_tags::float_image_data), FO, e_comm_tags::float_image_data); }
		if (opticsBuffer) { sendMPIMessage(image_comm, optics_start, slice_size, tagMap(e_comm_tags::float_image_data), FO, e_comm_tags::float_image_data); }

		if (!instance_ID) { // Director - receives last, directly apply to the buffer
			REPORT("Receive at Director" << instance_ID << " slice " << slice);
			if (maskPixelBuffer) { receiveMPIMessage(image_comm, mask_start, received_slice_size, tagMap(rmask), MPI_STATUSES_IGNORE, received_from, rmask); }
			if (phantomBuffer) { receiveMPIMessage(image_comm, phantom_start, received_slice_size, tagMap(rimg), MPI_STATUSES_IGNORE, received_from, rimg);  }
			if (opticsBuffer) { receiveMPIMessage(image_comm, optics_start, received_slice_size, tagMap(rimg), MPI_STATUSES_IGNORE, received_from, rimg); }
			//assert(received_slice_size == slice_size && received_from == instances - 1);
		}
	}
	free(mask_add);
	free(phantom_add);
	free(optics_add);
	DEBUG_REPORT("From #" << instance_ID << " to #" << FO << " done ");
}


size_t MPI_Communicator::cntOfAABBs(int FO, bool broadcast)
{
	e_comm_tags tag = e_comm_tags::count_AABB;
	uint64_t buffer [] = {0};
	int cnt = 1;
	DEBUG_REPORT("Request AABBS total from #" << FO << ((broadcast)?" (broadcast)":"") << " at #" << instance_ID);
	sendFO(buffer, 0, FO, e_comm_tags::get_count_AABB);
	//DEBUG_REPORT("Waiting for AABBs total from " << FO);
	if (broadcast) {
		receiveBroadcast(buffer, cnt, FO, e_comm_tags::count_AABB);
	} else {
		receiveFOMessage(buffer, cnt, FO, tag);
	}
	DEBUG_REPORT("AABBS total " << buffer[0] << " from #" << FO);
	return buffer[0];
}

void MPI_Communicator::sendCntOfAABBs(size_t count_AABBs, bool broadcast)
{
	size_t buffer [] = {count_AABBs};
	DEBUG_REPORT("Send AABBS total from #" << instance_ID << " total " << buffer[0] << ((broadcast)?" (broadcast)":""));
	if (broadcast) {
		sendBroadcast(buffer, 1, instance_ID, e_comm_tags::count_AABB);
	} else {
		sendDirector(buffer, 1, e_comm_tags::count_AABB);
	}
}


void MPI_Communicator::waitFor_publishAgentsAABBs() {
	waitSync(e_comm_tags::send_AABB);
}

void MPI_Communicator::waitFor_renderNextFrame() {
	waitSync(e_comm_tags::float_image_data);
}

void MPI_Communicator::sendNextID(int id) {
	e_comm_tags tag = e_comm_tags::next_ID;
	sendLastFO(&id, 1, tag); /* MISSING Agent ID, for now solved with this variable !!!*/
}

void MPI_Communicator::close() {
	MPI_Finalize();
}

#endif
