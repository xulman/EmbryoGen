#include "../Agents/AbstractAgent.hpp"
#include "../FrontOfficer.hpp"
#include <cassert>

FrontOfficer::FrontOfficer(ScenarioUPTR s,
                           const int myPortion,
                           const int allPortions)
    : scenario(std::move(s)), ID(myPortion),
      nextFOsID((myPortion + 1) % (allPortions + 1)), FOsCount(allPortions),
      nextAvailAgentID(1) {
	scenario->declareFOcontext(myPortion);
}

FrontOfficer::~FrontOfficer() {
	report::debugMessage(fmt::format("running the closing sequence"));
	report::debugMessage(
	    fmt::format("will remove {} active agents", agents.size()));
	report::debugMessage(
	    fmt::format("will remove {} shadow agents", shadowAgents.size()));
}

/** Never called in ST case, but could have some use in other implementations */
void FrontOfficer::init() {}

/** Never called in ST case, but could have some use in other implementations */
void FrontOfficer::execute() {}

void FrontOfficer::waitHereUntilEveryoneIsHereToo(
    const std::source_location&) const {}
void FrontOfficer::waitFor_publishAgentsAABBs() const {}
void FrontOfficer::waitForAllFOs(const std::source_location&) const {}

void FrontOfficer::exchange_AABBofAgents() {
	// in this ST particular implementation we do only update ourselves,
	// there is no other FO and Direktor doesn't care about this update
	for (const auto& [id, ag] : agents) {
		AABBs.emplace_back(ag->getAABB(), id, ag->getAgentTypeID());
		agentsAndBroadcastGeomVersions[id] = ag->getGeometry().version;
		registerThatThisAgentIsAtThisFO(id, getID());
	}
}

std::unique_ptr<ShadowAgent>
FrontOfficer::request_ShadowAgentCopy(const int /* agentID */,
                                      const int /* FOsID */) const {
	// this never happens here (as there is no other FO to talk to)
	assert(false);
	return nullptr;
}

void FrontOfficer::waitFor_renderNextFrame() const {}
