#include "../Agents/NucleusAgent.hpp"
#include "../DisplayUnits/SceneryBufferedDisplayUnit.hpp"
#include "../Geometries/Spheres.hpp"
#include "../util/Vector3d.hpp"
#include "common/Scenarios.hpp"
#include <fmt/core.h>

class myNucleusB : public NucleusAgent {
  public:
	myNucleusB(const int _ID,
	           const std::string& _type,
	           const Spheres& shape,
	           const float _currTime,
	           const float _incrTime)
	    : NucleusAgent(_ID, _type, shape, _currTime, _incrTime) {
		report::message(fmt::format("{} c'tor", getSignature()));
		cytoplasmWidth = 25.0f;
	}

	~myNucleusB(void) {
		report::message(fmt::format("{} d'tor", getSignature()));
	}

	void advanceAndBuildIntForces(const float gTime) override {
		if (gTime >= 2.0f && ID == 1) {
			// Officer->closeAgent(this);

			int IID = ID + 10;

			Spheres s(futureGeometry);
			s.updateCentre(0, s.getCentres()[0] + Vector3d<float>(0, 0, 5));
			s.updateCentre(1, s.getCentres()[1] + Vector3d<float>(0, 0, 5));
			myNucleusB* agA = new myNucleusB(IID++, "nucleusB", s,
			                                 currTime + incrTime, incrTime);
			agA->setDetailedDrawingMode(true);
			// Officer->startNewDaughterAgent(agA,ID);

			s.updateCentre(0, s.getCentres()[0] - Vector3d<float>(0, 0, 10));
			s.updateCentre(1, s.getCentres()[1] - Vector3d<float>(0, 0, 10));
			myNucleusB* agB = new myNucleusB(IID++, "nucleusB", s,
			                                 currTime + incrTime, incrTime);
			agB->setDetailedDrawingMode(true);
			// Officer->startNewDaughterAgent(agB,ID);

			Officer->closeMotherStartDaughters(this, agA, agB);
		}

#ifdef DEBUG
		// export forces for display:
		forcesForDisplay = forces;
#endif
		// increase the local time of the agent
		currTime += incrTime;
	}
};

//==========================================================================
void Scenario_pseudoDivision::initializeAgents(FrontOfficer* fo, int p, int) {
	if (p != 1) {
		report::message(
		    "Populating only the first FO (which is not this one).");
		return;
	}

	// to obtain a sequence of IDs for new agents...
	int ID = 1;

	const int howManyToPlace = 2;
	for (int i = 0; i < howManyToPlace; ++i) {
		// the wished position relative to [0,0,0] centre
		Vector3d<float> pos((float)i * 1.0f, 0, 50);

		// position is shifted to the scene centre
		pos.x += params.constants.sceneSize.x / 2.0f;
		pos.y += params.constants.sceneSize.y / 2.0f;
		pos.z += params.constants.sceneSize.z / 2.0f;

		// position is shifted due to scene offset
		pos.x += params.constants.sceneOffset.x;
		pos.y += params.constants.sceneOffset.y;
		pos.z += params.constants.sceneOffset.z;

		Spheres s(2);
		s.updateCentre(0, pos);
		s.updateRadius(0, 10.0f);
		s.updateCentre(1, pos + Vector3d<float>(0, 3, 0));
		s.updateRadius(1, 10.0f);

		myNucleusB* ag =
		    new myNucleusB(ID++, "nucleus", s, params.constants.initTime,
		                   params.constants.incrTime);
		ag->setDetailedDrawingMode(true);
		fo->startNewAgent(ag);
	}
}

void Scenario_pseudoDivision::initializeScene() {
	displays.registerDisplayUnit(
	    []() { return new SceneryBufferedDisplayUnit("localhost:8765"); });
}

SceneControls& Scenario_pseudoDivision::provideSceneControls() {
	SceneControls::Constants myConstants;
	myConstants.stopTime = 6.0f;

	return *(new SceneControls(myConstants));
}
