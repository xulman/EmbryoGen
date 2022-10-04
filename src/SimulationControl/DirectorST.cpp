#include "../Director.hpp"
#include <cassert>

class ImplementationData {
  public:
	ImplementationData(ScenarioUPTR s,
	                   int nextFO,
	                   int portion,
	                   int portionCount)
	    : FO(std::move(s), nextFO, portion, portionCount) {}
	FrontOfficer FO;
};

ImplementationData& get_data(std::shared_ptr<void>& obj) {
	return *static_cast<ImplementationData*>(obj.get());
}

Director::Director(std::function<ScenarioUPTR()> ScenarioFactory)
    : scenario(ScenarioFactory()),
      shallWaitForUserPromptFlag(scenario->params->shallWaitForUserPromptFlag) {
	scenario->declareDirektorContext();

	implementationData =
	    std::make_unique<ImplementationData>(ScenarioFactory(), 0, 1, 1);

	get_data(implementationData).FO.connectWithDirektor(this);
}

void Director::init() {
	ImplementationData& impl = get_data(implementationData);
	init1_SMP();         // init the simulation
	impl.FO.init1_SMP(); // create my part of the scene
	init2_SMP();         // init the simulation
	impl.FO.init2_SMP(); // populate the simulation
	init3_SMP();         // and render the first frame
}

int Director::getFOsCount() const { return 1; }
FrontOfficer& Director::getFirstFO() { return get_data(implementationData).FO; }