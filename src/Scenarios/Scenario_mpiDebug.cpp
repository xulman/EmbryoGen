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
report::debugMessage(fmt::format("empty example one-time adjustment..." ));
			}
		}
	};

	mySceneControl* ctrl = new mySceneControl(myConstants);
	return *ctrl;
}


void Scenario_mpiDebug::initializeScene()
{
report::debugMessage(fmt::format("disabling wait for key" ));
	params.disableWaitForUserPrompt();
}


void Scenario_mpiDebug::initializeAgents(FrontOfficer*,int p,int P)
{
report::debugMessage(fmt::format("FOs slice={}/{}" , p, P));

#ifdef DISTRIBUTED
	//code that gets compiled only in MPI (DISTRIBUTED) version

	int len;
	int rank, size;
	char node[MPI_MAX_PROCESSOR_NAME];

	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Get_processor_name(node,&len);

report::message(fmt::format("Hello world! from MPI true rank {} of {} on host {}" , rank, size, node));
#endif
}
