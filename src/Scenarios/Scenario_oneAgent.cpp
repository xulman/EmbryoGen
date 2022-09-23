#include "../Agents/NucleusAgent.hpp"
#include "../Geometries/Spheres.hpp"
#include "../util/report.hpp"
#include "common/Scenarios.hpp"
#include <fmt/core.h>

void Scenario_oneAgent::initializeAgents(FrontOfficer* fo,
                                         int part,
                                         int noOfParts) {
	report::debugMessage(fmt::format(
	    "Giving one agent to FO representiong part {}/{}", part, noOfParts));

	Vector3d<float> sceneCentre = params->constants.sceneSize;
	sceneCentre /= 2.0f;
	sceneCentre += params->constants.sceneOffset;

	Spheres s(1);
	s.updateCentre(0, sceneCentre + (0.5f - float(part % 2)) * float(part) *
	                                    Vector3d<float>(30.0f));
	s.updateRadius(0, 20);

	fo->startNewAgent(new NucleusAgent(part, "Init", s,
	                                   params->constants.initTime,
	                                   params->constants.incrTime));
}

void Scenario_oneAgent::initializeScene() { disks.enableImgMaskTIFFs(); }

std::unique_ptr<SceneControls> Scenario_oneAgent::provideSceneControls() const {
	config::scenario::ControlConstants c;
	c.stopTime = 1.6f;

	auto sc = std::make_unique<SceneControls>(c);
	sc->disableWaitForUserPrompt();

	return sc;
}
