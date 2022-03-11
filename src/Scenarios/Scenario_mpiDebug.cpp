#include "common/Scenarios.hpp"
#ifdef DISTRIBUTED
	#include <mpi.h>
#endif

SceneControls& Scenario_mpiDebug::provideSceneControls()
{
	//does 3 iterations (but w/o agents)
	SceneControls::Constants myConstants;
	myConstants.stopTime = myConstants.initTime + 3*myConstants.incrTime;

	class mySceneControl: public SceneControls
	{
	public:
		mySceneControl(Constants& c): SceneControls(c) {}

		void updateControls(const float currTime) override
		{
			//execute the following own code after every iteration

			if (currTime == 0.1f)
			{
				DEBUG_REPORT("empty example one-time adjustment...");
			}
		}
	};

	mySceneControl* ctrl = new mySceneControl(myConstants);
	return *ctrl;
}


void Scenario_mpiDebug::initializeScene()
{
	DEBUG_REPORT("disabling wait for key");
	params.disableWaitForUserPrompt();
}


void Scenario_mpiDebug::initializeAgents(FrontOfficer*,int p,int P)
{
	DEBUG_REPORT("FOs slice=" << p << "/" << P);

#ifdef DISTRIBUTED
	//code that gets compiled only in MPI (DISTRIBUTED) version

	int len;
	int rank, size;
	char node[MPI_MAX_PROCESSOR_NAME];

	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Get_processor_name(node,&len);

	REPORT("Hello world! from MPI true rank " << rank << " of " << size
	    << " on host " << node);
#endif
}
