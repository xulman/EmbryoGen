#ifndef DISTRIBUTEDCOMMUNICATOR_H
#define DISTRIBUTEDCOMMUNICATOR_H

#include "../Agents/AbstractAgent.h"
#include "../util/report.h"

typedef struct {
        Vector3d<G_FLOAT> minCorner;  // AABB minCorner from geometry
        Vector3d<G_FLOAT> maxCorner;  // AABB maxCorner from geometry
        int version; // Version from geometry itself
        int id;		 // Agent ID from agent
        int atype;	 // Agent Type ID, from agent
} t_aabb;

#define StringsImprintSize 256 //This should be moved to some common header file with strings

typedef struct {
		size_t hash;
		char value [StringsImprintSize];
} t_hashed_str;

#define DIRECTOR_RECV_MAX (1<<20) // Receive buffer for director-only communication
#define DIRECTOR_ID 0 			  // Main node
#define FO_INSTANCE_ANY 0		  // Any FO is OK

#define DISTRIBUTED_DEBUG


typedef enum {
	next_ID=0,
	new_agent=1,
	close_agent=2,
	update_parent=3,
	send_AABB=4,
	count_AABB=5,
	new_type=6,
	count_new_type=7,
	shadow_copy=8,
	render_frame=10,
	set_detailed_drawing=20,
	set_detailed_reporting=21,
	set_debug=0x23, // rendering debug
	next_stage=0x30, //FO finished
	unblock_FO=0x31, //FO can start
	finished=0x32, //All work done
	ACK=40,
	noop=80,
	unspecified=-1
} e_comm_tags;

class DistributedCommunicator
{
public:
		DistributedCommunicator()
		{
				instances = 1;
				instance_ID = 0;
				internal_agent_ID = 0;
				hasSent = false;
				shift=sizeof(int)*8 - 1; //2^31
		}

		inline int getInstanceID() { return instance_ID;  }
		inline int getNumberOfNodes() { return instances; }

		virtual void close() {}

		/*** Communication channel to the director ***/

		virtual int sendDirector(void *data, int count, e_comm_tags tag) = 0;

		virtual bool receiveAndProcessDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag) = 0;
		virtual bool receiveDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag, bool async=true) = 0;

		inline int sendACKtoDirector() {
			char id = 0;
			return sendDirector(&id, 0, e_comm_tags::ACK); // We could also store last tag and send it back instead of ACK
		}

		inline void receiveDirectorACK() {
			e_comm_tags tag = e_comm_tags::ACK; 		//Should we use ::unspecified instead?
			receiveDirectorMessage(director_buffer, director_buffer_size, tag);
		}

		virtual void waitSync() {}

		inline void unblockNextIfSent() {
			if (hasSent) {
				hasSent=false;
				sendNextFO(NULL, 0, e_comm_tags::next_stage);
			}
		}

		/*** Front officer communication channels */

		virtual e_comm_tags detectFOMessage(bool async=true) = 0;
		virtual bool receiveFOMessage(void * buffer, int &recv_size, int & instance_ID,  e_comm_tags &tag) = 0; //Single reception step, for response messages
		virtual int sendFO(void *data, int count, int instance_ID, e_comm_tags tag) = 0;

		inline int sendLastFO(void *data, int count, e_comm_tags tag) {
			return sendFO(data, count, lastFOID, tag);
		}

		inline int sendACKtoLastFO() {
			char id = 0;
			return sendLastFO(&id, 0, e_comm_tags::ACK); // We could also store last tag and send it back instead of ACK
		}

		inline void receiveFOACK(int FO) {
			e_comm_tags tag = e_comm_tags::ACK; 		//Should we use ::unspecified instead?
			receiveFOMessage(director_buffer, director_buffer_size, FO, tag);
		}


		inline int sendNextFO(void *data, int count, e_comm_tags tag) {
			return sendFO(data, count, (instance_ID+1) % instances, tag);
		}

		inline virtual int sendPrevFO(void *data, int count, e_comm_tags tag) { // Is this method needed at all?
			return sendFO(data, count, (instance_ID+instances-1) % instances, tag);
		}

		inline int receiveAndProcessNextFOMessages(e_comm_tags tag = e_comm_tags::unspecified) {
			int toFO = (instance_ID + 1) % instances;
			return receiveFOMessage(director_buffer, director_buffer_size, toFO, tag);
		}

		inline int receiveAndProcessPrevFOMessages(e_comm_tags tag = e_comm_tags::unspecified) {
			int toFO = (instance_ID + instances - 1) % instances;
			return receiveFOMessage(director_buffer, director_buffer_size, toFO, tag);
		}

		/*** Broadcast channel ***/
		virtual bool sendBroadcast(void *data, int count, int sender_id, e_comm_tags tag) = 0;
		virtual bool receiveBroadcast(void *data, int & count, int sender_id, e_comm_tags tag) = 0;

		/*** Specific communication methods ***/

		virtual int getNextAvailAgentID() = 0;
		/*{
			return (instance_ID << shift) | (++internal_agent_ID); //Recalculate better to take into account milions of cells!!!
		}*/


		virtual void startNewAgent(const int newAgentID, const int associatedFO, const bool wantsToAppearInCTCtracksTXTfile = true) = 0;
		virtual void closeAgent(const int agentID, const int associatedFO) = 0;
		virtual void startNewDaughterAgent(const int childID, const int parentID) = 0;

		virtual void publishAgentsAABBs(int FO) = 0;
		virtual void waitFor_publishAgentsAABBs() = 0;
		virtual void waitFor_renderNextFrame() = 0;

		virtual void sendNextID(int id) = 0;

		virtual void setAgentsDetailedDrawingMode(int FO, int agentID, bool state) = 0;
		virtual void setAgentsDetailedReportingMode(int FO, int agentID, bool state) = 0;

		virtual size_t cntOfAABBs(int FO, bool broadcast=false ) = 0;
		virtual void sendCntOfAABBs(size_t count_AABBs, bool broadcast=false) = 0;

		virtual void renderNextFrame(int FO) = 0;

		inline const char * tagName(e_comm_tags tag) {
			static char last_str [64] = { 0 };
			switch (tag) {
				case e_comm_tags::next_ID:
					return "Next ID";
				case e_comm_tags::new_agent:
					return "New Agent";
				case e_comm_tags::update_parent:
					return "Update Parent";
				case e_comm_tags::close_agent:
					return "Close Agent";
				case e_comm_tags::count_AABB:
					return "Count AABB";
				case e_comm_tags::send_AABB:
					return "Send AABB";
				case e_comm_tags::next_stage:
					return "Next Stage";
				case e_comm_tags::unblock_FO:
					return "Unblock FO";
				case e_comm_tags::finished:
					return "Calculation Finished";
				case e_comm_tags::noop:
					return "No operation";
				case e_comm_tags::ACK:
					return "ACK";
				case e_comm_tags::set_detailed_drawing:
					return "Set detailed drawing";
				case e_comm_tags::set_detailed_reporting:
					return "Set detailed reporting";
				case e_comm_tags::set_debug:
					return "Set rendering debug";
				case e_comm_tags::new_type:
					return "New type";
				case e_comm_tags::count_new_type:
					return "New type count";
				case e_comm_tags::shadow_copy:
					return "Shadow copy";
				case e_comm_tags::render_frame:
					return "Render frame";
				case e_comm_tags::unspecified:
					return "ANY";
				default:				// shadow_copy, render frame, // Conversions would be slow?
					sprintf(last_str, "? (%i)%c", tag, '\0');
					return last_str;
			}
		}

protected:
		int instance_ID;
		int instances;
		int next_agent;
		int internal_agent_ID;
		int lastFOID;
		int hasSent;

		int shift; // Shift ID by given number of bits, to reduce communication for getNextAvailAgentID, maybe rework later
		static e_comm_tags messageType; // Message type enum for sending/receiving individual distributed messages

		char director_buffer [DIRECTOR_RECV_MAX] = {0};
		int director_buffer_size = DIRECTOR_RECV_MAX;


};

#ifdef DISTRIBUTED
#include <mpi.h>

class MPI_Communicator : public DistributedCommunicator
{
public:
		MPI_Communicator(int *argc=NULL, char ***argv=NULL) : DistributedCommunicator()
		{
				lastFOID = -1;
				init(argc, argv);
		}

		~MPI_Communicator();
		inline const char * getProcessorName() { return processor_name;  }

		virtual int getNextAvailAgentID();
		virtual void startNewAgent(const int newAgentID, const int associatedFO, const bool wantsToAppearInCTCtracksTXTfile = true);
		virtual void closeAgent(const int agentID, const int associatedFO);
		virtual void startNewDaughterAgent(const int childID, const int parentID);

		virtual void publishAgentsAABBs(int FO);
		virtual void waitFor_publishAgentsAABBs();
		virtual void waitFor_renderNextFrame();

		virtual void sendNextID(int id);

		virtual void setAgentsDetailedDrawingMode(int FO, int agentID, bool state);
		virtual void setAgentsDetailedReportingMode(int FO, int agentID, bool state);
		virtual size_t cntOfAABBs(int FO, bool broadcast=false);
		virtual void sendCntOfAABBs(size_t count_AABB, bool broadcast=false);

		virtual void renderNextFrame(int FO);

		/*** Communication channel to the director ***/
		virtual int sendDirector(void *data, int count, e_comm_tags tag) {
			return sendMPIMessage(director_comm, data, count, tagMap(tag), 0, tag);
		}

		virtual e_comm_tags detectFOMessage(bool async=true);

		virtual int sendFO(void *data, int count, int instance_ID, e_comm_tags tag) {
			return sendMPIMessage(director_comm, data, count, tagMap(tag), instance_ID, tag);
		}

		//boot receiveAndProcessDirectorMessagesForDirector(Director & director....
		virtual bool receiveAndProcessDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag); //Probably not needed to run in cycle if notifications separate, only director needs cycle
		virtual bool receiveDirectorMessage(void * buffer, int &recv_size, e_comm_tags &tag, bool async=true); //Single reception step, for request-response messages

		/*** Front officer communication channels */
		virtual bool receiveFOMessage(void * buffer, int &recv_size, int & instance_ID,  e_comm_tags &tag); //Single reception step, for response messages

		virtual void waitSync() {
			e_comm_tags tag = e_comm_tags::next_stage;
			debugMPIComm("WaitSync", director_comm, -1, instance_ID,  e_comm_tags::unspecified);
			if (instance_ID) {
				sendDirector(no_message, 0, tag);
			}
			MPI_Barrier(director_comm);
			/*if (instance_ID != 0) {
				receiveDirectorMessage(director_buffer, director_buffer_size, tag); //TBD: Director or broadcast message?
			}*/
			debugMPIComm("WaitSyncEnd", director_comm, -1, instance_ID,  e_comm_tags::unspecified);
		}

		virtual void close();

protected:
		char processor_name[MPI_MAX_PROCESSOR_NAME];
		static char no_message[1];
		int name_len;

		int init(int *argc, char ***argv);

		inline void debugMPIComm(const char* what, MPI_Comm comm, int items, int peer=MPI_ANY_SOURCE, e_comm_tags tag = e_comm_tags::unspecified) {
#ifdef DISTRIBUTED_DEBUG
			int rlen=64;
			char cname [64] = {0};
			MPI_Comm_get_name(comm, cname, &rlen);
			REPORT(what << " MPI message at: " << instance_ID << " Via: " << cname <<  " Peer: " << peer <<  " Items in message: " << items << " Tag: " << tagName(tag));
#endif
		}

		inline int sendMPIMessage(MPI_Comm comm, void *data, int items, MPI_Datatype datatype, int peer=0, e_comm_tags tag = e_comm_tags::unspecified) {
			int tag2 = (tag == e_comm_tags::unspecified)?MPI_ANY_TAG:tag;
			debugMPIComm("Send", comm, items, peer, tag);
			hasSent = true;
			return MPI_Send(data, items, datatype, peer, tag2, comm);
		}

		int sendMPIBroadcast(MPI_Comm comm, void *data, int items, MPI_Datatype datatype) { //AABB from each FO to all as an example
			debugMPIComm("Broadcast", comm, items, instance_ID, e_comm_tags::unspecified);
			return MPI_Bcast(data, items, datatype, instance_ID, comm);
		}

		inline int receiveMPIMessage(MPI_Comm comm, void * data,  int & items, MPI_Datatype datatype, MPI_Status *status = MPI_STATUSES_IGNORE , int peer=MPI_ANY_SOURCE, e_comm_tags tag = e_comm_tags::unspecified) {
			debugMPIComm("Ask to receive", comm, items, peer, tag);

			int state = MPI_Recv(data, items, datatype, peer, tag, comm, status);
			if (state == MPI_SUCCESS) {
				state = MPI_Get_count(status, datatype, &items);
			}
			debugMPIComm("Received", comm, items, peer, tag);
			return state;
		}

		inline MPI_Datatype tagMap(e_comm_tags tag) {
			switch (tag) {
				case e_comm_tags::next_ID:
				case e_comm_tags::new_agent: //int, int, bool - special datype?
				case e_comm_tags::update_parent:
				case e_comm_tags::close_agent:
					return MPI_INT64_T;				//Really? Or MPI_INT64_T or MPI_UINT64_T?
				case e_comm_tags::count_new_type:
				case e_comm_tags::count_AABB:
					return MPI_UINT64_T;
				case e_comm_tags::send_AABB:
					return MPI_AABB;			//Broadcast First Types, then AABBs
				case e_comm_tags::next_stage:
				case e_comm_tags::noop:
				case e_comm_tags::unblock_FO:
				case e_comm_tags::finished:
				case e_comm_tags::ACK:
					return MPI_CHAR;
				//case e_comm_tags:: : //Image size -> define image type dynamically after start, each frame!!!
				//case e_comm_tags::set_detailed_drawing: case e_comm_tags::set_debug: //could be also MPI_C_BOOL
				//case e_comm_tags::new_type // byte array or a special MPI structure? - UINT64 + 256*Char !!!
				default:				// shadow_copy, render frame, // Conversions would be slow?
					return MPI_BYTE;
			}
		}

		inline MPI_Comm tagCommMap(e_comm_tags tag) {
			switch (tag) {
				case e_comm_tags::send_AABB:
					return aabb_comm;			//Broadcast First Types, then AABBs
				case e_comm_tags::count_new_type:
				case e_comm_tags::new_type:
					return type_comm;
				default:
					return MPI_COMM_WORLD;
			}
		}


		bool detectMPIMessage(MPI_Comm comm, int peer, e_comm_tags & tag, bool async=true);

		/*** Broadcast channel ***/
		virtual bool sendBroadcast(void *data, int items, int sender_id, e_comm_tags tag)
		{
			debugMPIComm("Send Broadcast", tagCommMap(tag), items, sender_id, tag);
			return MPI_Bcast(data, items, tagMap(tag), sender_id, tagCommMap(tag));
		}

		virtual bool receiveBroadcast(void *data, int & items, int sender_id, e_comm_tags tag)
		{
			debugMPIComm("Receive Broadcast", tagCommMap(tag), items, sender_id, tag);
			return MPI_Bcast(data, items, tagMap(tag), sender_id, tagCommMap(tag));
		}

		//int receiveAndProcessMPIBroadcasts(e_comm_tags tag = e_comm_tags::unspecified);


		// Frame rendering not necessary to wait for token
		// for with ID to distunguish send/receive for AABB? Token passing better (scene with more independent part)

	    MPI_Datatype MPI_AABB;
		MPI_Datatype MPI_VECTOR3D;
		MPI_Datatype MPI_AGENTTYPE;
		//MPI_Comm id_sender; //Back Channel from Director (sending new IDs)
		MPI_Comm token_comm; // Channel to synchronize FO according to token passing description
		MPI_Comm director_comm; //Channel to/from Director
		MPI_Comm aabb_comm; //AABB broadcasting exchange
		MPI_Comm type_comm; //New agent type exchange

};
#endif /*DISTRIBUTED*/

#endif
