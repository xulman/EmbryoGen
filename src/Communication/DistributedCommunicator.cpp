#include "DistributedCommunicator.h"
#include <chrono>
#include <thread>

// ID v batchi po 1000 a pak zpracovat? Ale co newAgent a ParentalLink, to stejně nepomůže. -> Do budoucna, zatím ne.


#ifdef DISTRIBUTED
MPI_Communicator::~MPI_Communicator() {
// Did not work correctly:	MPI_Finalize();
}

int MPI_Communicator::init(int *argc, char ***argv) {
	int cshift = 1, icnt;
	int state = MPI_Init(argc, argv);

	if (state != MPI_SUCCESS) {  return state; }

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
    MPI_Datatype aabb_types[] = {MPI_VECTOR3D, MPI_VECTOR3D, MPI_INT, MPI_INT, MPI_INT};


    const int agentt_count = 2;
    int agentt_blocks[] = {1, StringsImprintSize};
    MPI_Aint agentt_displ[] = {offsetof(t_hashed_str, hash), offsetof(t_hashed_str, value)};
    MPI_Datatype agentt_types[] = {MPI_INT64_T, MPI_CHAR};
    MPI_Type_create_struct(agentt_count, agentt_blocks, agentt_displ, agentt_types, &MPI_AGENTTYPE);
    MPI_Type_commit(&MPI_AGENTTYPE);

    MPI_Type_create_struct(aabb_count, aabb_blocks, aabb_displ, aabb_types, &MPI_AABB);
    state = MPI_Type_commit(&MPI_AABB);

	director_comm = MPI_COMM_WORLD;
	MPI_Comm_dup(MPI_COMM_WORLD, &director_comm);
	MPI_Comm_set_name(director_comm, "Director_comm");
	MPI_Comm_split(MPI_COMM_WORLD, 1, instance_ID, &id_sender);
	MPI_Comm_dup(MPI_COMM_WORLD, &token_comm);
	//MPI_Comm_split(MPI_COMM_WORLD, 1, instance_ID, &token_comm);

	for (cshift=1, icnt=instances; icnt != 0 ; icnt = icnt	 >> 1) {
		cshift++;
	}
	shift = sizeof(int)*8 - cshift;
	DEBUG_REPORT("Shift agent ID by " << shift << " bits\n");

    MPI_Get_processor_name(processor_name, &name_len);
	return state;
}

bool MPI_Communicator::detectMPIMessage(MPI_Comm comm, int peer, e_comm_tags & tag) {
	int flag;
	MPI_Status status;
	int MPI_tag = (tag == e_comm_tags::unspecified)?MPI_ANY_TAG:tag;

	int state = MPI_Iprobe(peer, MPI_tag, comm, &flag, &status);

	if (state != MPI_SUCCESS) { return false; }
	if (flag) {
		//debugMPIComm("Poll", comm, flag, peer, tag);
		tag = (e_comm_tags) status.MPI_TAG;
	}

	return flag; // Message available
}

bool MPI_Communicator::receiveAndProcessDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag) {
	MPI_Status status;
	bool result;
	int state;

//	do { --
	result = detectMPIMessage(director_comm, DIRECTOR_ID, tag);
	if (result) {
		return receiveDirectorMessage(buffer, recv_size, tag);
	}
//	std::this_thread::sleep_for((std::chrono::milliseconds)1); // Check
//	} while (!result);
	return false;
}

bool MPI_Communicator::receiveDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag) {
	MPI_Status status;
	bool result;
	int state;

	state = receiveMPIMessage(director_comm, buffer, recv_size, MPI_CHAR, &status, DIRECTOR_ID, tag);
	if (state == MPI_SUCCESS) {
		tag = (e_comm_tags) status.MPI_TAG;
		return true;
	}
	return false;
}

e_comm_tags MPI_Communicator::detectFOMessage() {
	e_comm_tags tag = e_comm_tags::unspecified;
	if (detectMPIMessage(director_comm, MPI_ANY_SOURCE, tag)) {
		return tag;
	} else {
		return e_comm_tags::unspecified;
	}
}

bool MPI_Communicator::receiveFOMessage(void * buffer, int &recv_size, int & instance_ID, e_comm_tags &tag) {
	MPI_Status status;
	bool result;
	int state;

	if (instance_ID == 0) { instance_ID = MPI_ANY_SOURCE;}

	state = receiveMPIMessage(director_comm, buffer, recv_size, MPI_CHAR, &status, instance_ID, tag);
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
	sendDirector(NULL, 0, tag);
	receiveDirectorMessage(buffer, id_cnt, tag);
	DEBUG_REPORT("From FO " << instance_ID << " to Director: get new agent ID=" << buffer[0]);
	return buffer[0];
}

void MPI_Communicator::startNewAgent(const int newAgentID, const int associatedFO, const bool wantsToAppearInCTCtracksTXTfile)
{
	int buffer [] = {newAgentID, associatedFO, wantsToAppearInCTCtracksTXTfile};
	DEBUG_REPORT("From FO " << associatedFO << " to Director: tart new agent ID=" << newAgentID << ((wantsToAppearInCTCtracksTXTfile)?" (CTC)":"") );
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
	//fprintf(stderr, "MPI_SEND to %i startNewDaughterAgent %i of agent %i\n", childID, parentID);
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
	sendFO(buffer, 0, FO, e_comm_tags::send_AABB);
	receiveFOACK(FO);
}

void MPI_Communicator::renderNextFrame(int FO)
{
	int buffer [] = {0};
	sendFO(buffer, 0, FO, e_comm_tags::render_frame);
	receiveFOACK(FO);
}


size_t MPI_Communicator::cntOfAABBs(int FO)
{
	e_comm_tags tag = e_comm_tags::count_AABB;
	int buffer [] = {0};
	int id_cnt = 1;
	sendFO(buffer, 0, FO, e_comm_tags::count_AABB);
	receiveFOMessage(buffer, id_cnt, FO, tag);
	return buffer[0];
}

void MPI_Communicator::sendCntOfAABBs(size_t count_AABBs)
{
	e_comm_tags tag = e_comm_tags::count_AABB;
	size_t buffer [] = {count_AABBs};
	sendDirector(buffer, 1, e_comm_tags::update_parent);
}


void MPI_Communicator::waitFor_publishAgentsAABBs() {
	waitSync();
}

void MPI_Communicator::waitFor_renderNextFrame() {
	waitSync();
}

void MPI_Communicator::sendNextID(int id) {
	e_comm_tags tag = e_comm_tags::next_ID;
	int id_cnt = 1;
	sendLastFO(&id, 1, tag); /* MISSING Agent ID, for now solved with this variable !!!*/
}

void MPI_Communicator::close() {
	MPI_Finalize();
}

#endif
