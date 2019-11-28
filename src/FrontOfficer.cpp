#include "FrontOfficer.h"

void FrontOfficer::execute(void)
{
	REPORT("FO #" << ID << " is ready to start simulating");

	//main sim loop

	//this was promised to happen after every simulation round is over
	scenario.updateScene(1.0f); //TODO: time is wrong
}
