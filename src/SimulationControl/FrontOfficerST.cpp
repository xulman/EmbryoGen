#include "../Agents/AbstractAgent.hpp"
#include "../Director.hpp"
#include "../FrontOfficer.hpp"

FrontOfficer::FrontOfficer(ScenarioUPTR s,
                           const int myPortion,
                           const int allPortions)
    : scenario(std::move(s)), ID(myPortion),
      nextFOsID((myPortion + 1) % (allPortions + 1)), FOsCount(allPortions) {
	scenario->declareFOcontext(myPortion);
}

void FrontOfficer::waitHereUntilEveryoneIsHereToo() {}

/** Never called in ST case, but could have some use in other implementations */
void FrontOfficer::init() {}

/** Never called in ST case, but could have some use in other implementations */
void FrontOfficer::execute() {}

int FrontOfficer::request_getNextAvailAgentID() {
	return Direktor->getNextAvailAgentID();
}

void FrontOfficer::request_startNewAgent(
    const int newAgentID,
    const int associatedFO,
    const bool wantsToAppearInCTCtracksTXTfile) {
	Direktor->startNewAgent(newAgentID, associatedFO,
	                        wantsToAppearInCTCtracksTXTfile);
}

void FrontOfficer::request_closeAgent(const int agentID,
                                      const int associatedFO) {
	Direktor->closeAgent(agentID, associatedFO);
}

void FrontOfficer::request_updateParentalLink(const int childID,
                                              const int parentID) {
	Direktor->startNewDaughterAgent(childID, parentID);
}

void FrontOfficer::waitFor_publishAgentsAABBs() {}

void FrontOfficer::notify_publishAgentsAABBs(const int /* FOsID */) {
	// once our broadcasting is over, notify next FO to do the same
	//(or notify the Direktor if we're the last on the round-robin-chain)

	// in this SMP particular implementation we do nothing because
	// there is no other FO, and the Direktor called us directly

	// in MPI case, tell the next FO (or the Direktor)
	// MPI_signalThisEvent_to( FOsID );
}

void FrontOfficer::broadcast_AABBofAgent(const ShadowAgent& ag) {
	// this shall tell all (including this one) FOs the AABB of the given agent,
	// the Direktor actually does not care

	// in this SMP particular implementation we do only update ourselves,
	// there is no other FO and Direktor doesn't care about this update
	//
	// this code essentially supplies the work of respond_AABBofAgent() in the
	// SMP world
	AABBs.emplace_back(ag.getAABB(), ag.getID(), ag.getAgentTypeID());
	agentsAndBroadcastGeomVersions[ag.getID()] = ag.getGeometry().version;
	registerThatThisAgentIsAtThisFO(ag.getID(), this->ID);

	/*
	//here, according to doc/agentTypeDictionary.txt
	//this method shall have one extra param: noOfNewAgentTypes
	//but it will not be used in this case
	*/
}

ShadowAgent* FrontOfficer::request_ShadowAgentCopy(const int /* agentID */,
                                                   const int /* FOsID */) {
	// this never happens here (as there is no other FO to talk to)
	return NULL;

	// MPI world:
	// contacts FO at FOsID telling it that it needs agentID's ShadowAgent back

	// gives: agentID, this->ID (to tell the other party whom to talk back)
	// waits
	// gets : Geometry data, agentID and agentType

	/* fake data!
	Geometry*   gotThisGeom      = new Spheres(4);
	int         gotThisAgentID   = 10;
	std::string gotThisAgentType = "fake agent";
	return new ShadowAgent(*gotThisGeom, gotThisAgentID,gotThisAgentType);
	*/
}

void FrontOfficer::waitFor_renderNextFrame(const int /*FOsID*/) {
	// normally, it should move the content of the images from this FO
	// into Direktor's images... but instead we render directly into
	// Direktor's images... see FrontOfficer::renderNextFrame()
}

void FrontOfficer::request_renderNextFrame(const int /* FOsID */) {
	// does nothing in the SMP

	// MPI world:

	// this is the counterpart to the waitFor_renderNextFrame()
	// one sends out the images in this order: mask, phantom, optics
}