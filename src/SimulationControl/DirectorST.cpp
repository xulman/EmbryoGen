#include "../Director.hpp"
#include <cassert>
#define UNUSED(x) (void)x

class ImplementationData {
  public:
	ImplementationData(ScenarioUPTR s, int portion, int portionCount)
	    : FO(std::move(s), portion, portionCount) {}
	FrontOfficer FO;
};

ImplementationData& get_data(const std::shared_ptr<void>& obj) {
	return *static_cast<ImplementationData*>(obj.get());
}

Director::Director(std::function<ScenarioUPTR()> ScenarioFactory)
    : scenario(ScenarioFactory()),
      shallWaitForUserPromptFlag(scenario->params->shallWaitForUserPromptFlag) {
	scenario->declareDirektorContext();

	implementationData =
	    std::make_unique<ImplementationData>(ScenarioFactory(), 1, 1);

	get_data(implementationData).FO.connectWithDirektor(this);
}

void Director::init() {
	ImplementationData& impl = get_data(implementationData);
	init1();         // init the simulation
	impl.FO.init1(); // create my part of the scene
	init2();         // init the simulation
	impl.FO.init2(); // populate the simulation
	init3();         // and render the first frame
}

int Director::getFOsCount() const { return 1; }
FrontOfficer& Director::getFirstFO() { return get_data(implementationData).FO; }

void Director::prepareForUpdateAndPublishAgents() {
	get_data(implementationData).FO.prepareForUpdateAndPublishAgents();
}

void Director::waitHereUntilEveryoneIsHereToo() const {}

void Director::notify_publishAgentsAABBs() const {
	get_data(implementationData).FO.updateAndPublishAgents();
}

void Director::waitFor_publishAgentsAABBs() const {}

void Director::postprocessAfterUpdateAndPublishAgents() const {
	get_data(implementationData).FO.postprocessAfterUpdateAndPublishAgents();
}

std::vector<std::size_t> Director::request_CntsOfAABBs() const {
	return {get_data(implementationData).FO.getSizeOfAABBsList()};
}

void Director::request_renderNextFrame() const {
	SceneControls& sc = *scenario->params;
	get_data(implementationData)
	    .FO.renderNextFrame(sc.imgMask, sc.imgPhantom, sc.imgOptics);
}

void Director::waitFor_renderNextFrame() const {}

void Director::notify_setDetailedDrawingMode(int FOsID,
                                             int agentID,
                                             bool state) const {
	UNUSED(FOsID);
	assert(FOsID == 1);
	get_data(implementationData)
	    .FO.setAgentsDetailedDrawingMode(agentID, state);
}

void Director::notify_setDetailedReportingMode(int FOsID,
                                               int agentID,
                                               bool state) const {
	UNUSED(FOsID);
	assert(FOsID == 1);
	get_data(implementationData)
	    .FO.setAgentsDetailedReportingMode(agentID, state);
}

void Director::broadcast_setRenderingDebug(bool setFlagToThis) const {
	get_data(implementationData).FO.setSimulationDebugRendering(setFlagToThis);
}

void Director::broadcast_executeInternals() const {
	get_data(implementationData).FO.executeInternals();
}

void Director::broadcast_executeExternals() const {
	get_data(implementationData).FO.executeExternals();
}

void Director::broadcast_executeEndSub1() const {
	get_data(implementationData).FO.executeEndSub1();
}

void Director::broadcast_executeEndSub2() const {
	get_data(implementationData).FO.executeEndSub2();
}