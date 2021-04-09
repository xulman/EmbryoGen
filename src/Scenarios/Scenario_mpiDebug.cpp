#include "common/Scenarios.h"

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
	DEBUG_REPORT("rank=" << p << "/" << P);

#ifdef DISTRIBUTED
	//code that gets compiled only in MPI (DISTRIBUTED) version
#endif
}
