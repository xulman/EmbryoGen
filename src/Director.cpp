#include "Director.h"

void Director::execute(void)
{
	//main sim loop
	bool renderingNow = true;
	trigger_publishGeometry();

	if (renderingNow)
	{
		trigger_renderNextFrame();
		scenario.doPhaseIIandIII();
	}

	//this was promised to happen after every simulation round is over
	scenario.updateScene(1.0f); //TODO: time is wrong
}
