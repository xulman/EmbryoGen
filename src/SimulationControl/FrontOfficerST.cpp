#include "../Agents/AbstractAgent.hpp"
#include "../FrontOfficer.hpp"

FrontOfficer::FrontOfficer(ScenarioUPTR s,
                           const int myPortion,
                           const int allPortions)
    : scenario(std::move(s)), ID(myPortion),
      nextFOsID((myPortion + 1) % (allPortions + 1)), FOsCount(allPortions),
      nextAvailAgentID(0) {
	scenario->declareFOcontext(myPortion);
}

/** Never called in ST case, but could have some use in other implementations */
void FrontOfficer::init() {}

/** Never called in ST case, but could have some use in other implementations */
void FrontOfficer::execute() {}

void FrontOfficer::waitHereUntilEveryoneIsHereToo() const {}
void FrontOfficer::waitFor_publishAgentsAABBs() const {}

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
	return nullptr;
}

void FrontOfficer::waitFor_renderNextFrame() const {}
