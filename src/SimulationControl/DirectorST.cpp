#include "../Director.hpp"
#include <cassert>

Director::Director(std::function<ScenarioUPTR()> ScenarioFactory,
                   const int firstFO,
                   const int allPortions)
    : scenario(ScenarioFactory()), firstFOsID(firstFO), FOsCount(allPortions),
      shallWaitForUserPromptFlag(scenario->params->shallWaitForUserPromptFlag) {
	scenario->declareDirektorContext();

	FOs.emplace_back(ScenarioFactory(), 0, 1, 1);
	FOs[0].connectWithDirektor(this);
	//  TODO: create an extra thread to execute/service the respond_...()
	//  methods
}

void Director::init() {
	assert(FOs.size() == 1);
	FrontOfficer& FO = FOs[0];
	init1_SMP();    // init the simulation
	FO.init1_SMP(); // create my part of the scene
	init2_SMP();    // init the simulation
	FO.init2_SMP(); // populate the simulation
	init3_SMP();    // and render the first frame
}