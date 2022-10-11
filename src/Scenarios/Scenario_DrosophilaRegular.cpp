#include "../Agents/Nucleus4SAgent.hpp"
#include "../Agents/ShapeHinter.hpp"
#include "../Agents/TrajectoriesHinter.hpp"
#include "../DisplayUnits/SceneryBufferedDisplayUnit.hpp"
#include "../Geometries/ScalarImg.hpp"
#include "../Geometries/VectorImg.hpp"
#include "../Geometries/util/SpheresFunctions.hpp"
#include "../util/Vector3d.hpp"
#include "common/Scenarios.hpp"

class GrowableNucleusReg : public Nucleus4SAgent {
  public:
	GrowableNucleusReg(const int _ID,
	                   const std::string& _type,
	                   const Spheres& shape,
	                   const float _currTime,
	                   const float _incrTime)
	    : Nucleus4SAgent(_ID, _type, shape, _currTime, _incrTime) {}

	float startGrowTime = 99999999.f;
	float stopGrowTime = 99999999.f;

  protected:
	int incrCnt = 0;

	void adjustGeometryByIntForces() override {
		// adjust the shape at first
		if (currTime >= startGrowTime && currTime <= stopGrowTime &&
		    incrCnt < 30) {
			//"grow factor"
			const float dR = 0.05f;     // radius
			const float dD = 1.8f * dR; // diameter

			// grow the current geometry
			SpheresFunctions::grow4SpheresBy(futureGeometry, dR, dD);

			// also update the expected distances
			for (int i = 1; i < 4; ++i)
				centreDistance[i - 1] += dD;

			// emergency break...
			++incrCnt;
		}

		// also call the upstream original method
		Nucleus4SAgent::adjustGeometryByIntForces();
	}
};

//==========================================================================
void Scenario_DrosophilaRegular::initializeAgents(FrontOfficer* fo,
                                                  int p,
                                                  int) {
	if (p != 1) {
		report::message(fmt::format(
		    "Populating only the first FO (which is not this one)."));
		return;
	}

	// stepping in all directions -> influences the final number of nuclei
	const float dx = 14.0f;

	// longer axis x
	// symmetric/short axes y,z

	const float Xside = (0.90f * params->constants.sceneSize.x) / 2.0f;
	const float YZside = (0.75f * params->constants.sceneSize.y) / 2.0f;

	for (float z = -Xside; z <= +Xside; z += dx) {
		// radius at this z position
		const float radius = YZside * std::sin(std::acos(std::abs(z) / Xside));

		const int howManyToPlace = (int)ceil(6.28f * radius / dx);
		for (int i = 0; i < howManyToPlace; ++i) {
			const float ang = float(i) / float(howManyToPlace);

			// the wished position relative to [0,0,0] centre
			Vector3d<float> axis(0, std::cos(ang * 6.28f),
			                     std::sin(ang * 6.28f));
			Vector3d<float> pos(z, radius * axis.y, radius * axis.z);

			// position is shifted to the scene centre
			pos.x += params->constants.sceneSize.x / 2.0f;
			pos.y += params->constants.sceneSize.y / 2.0f;
			pos.z += params->constants.sceneSize.z / 2.0f;

			// position is shifted due to scene offset
			pos.x += params->constants.sceneOffset.x;
			pos.y += params->constants.sceneOffset.y;
			pos.z += params->constants.sceneOffset.z;

			Spheres s(4);
			s.updateCentre(0, pos);
			s.updateRadius(0, 3.0f);
			s.updateCentre(1, pos + 6.0f * axis);
			s.updateRadius(1, 5.0f);
			s.updateCentre(2, pos + 12.0f * axis);
			s.updateRadius(2, 5.0f);
			s.updateCentre(3, pos + 18.0f * axis);
			s.updateRadius(3, 3.0f);

			auto ag = std::make_unique<GrowableNucleusReg>(
			    fo->getNextAvailAgentID(), "nucleus growable regular", s,
			    params->constants.initTime, params->constants.incrTime);
			ag->startGrowTime = 1.0f;
			fo->startNewAgent(std::move(ag));
		}
	}

	//-------------
	// now, read the mask image and make it a shape hinter...
	i3d::Image3d<i3d::GRAY8> initShape("../DrosophilaYolk_mask_lowerRes.tif");
	ScalarImg m(initShape, ScalarImg::DistanceModel::ZeroIN_GradOUT);
	// m.saveDistImg("GradIN_ZeroOUT.tif");

	// finally, create the simulation agent to register this shape
	auto ag = std::make_unique<ShapeHinter>(fo->getNextAvailAgentID(), "yolk",
	                                        m, params->constants.initTime,
	                                        params->constants.incrTime);
	fo->startNewAgent(std::move(ag), false);

	//-------------
	auto at = std::make_unique<TrajectoriesHinter>(
	    fo->getNextAvailAgentID(), "trajectories", initShape,
	    VectorImg::ChoosingPolicy::avgVec, params->constants.initTime,
	    params->constants.incrTime);

	// the trajectories hinter:
	at->talkToHinter().readFromFile("../DrosophilaYolk_movement.txt",
	                                Vector3d<float>(2.f), 10.0f, 10.0f);
	report::message(fmt::format("Timepoints: {}, Tracks: {}",
	                            at->talkToHinter().size(),
	                            at->talkToHinter().knownTracks.size()));

	fo->startNewAgent(std::move(at), false);
}

void Scenario_DrosophilaRegular::initializeScene() {
	displays.registerDisplayUnit([]() {
		return std::make_unique<SceneryBufferedDisplayUnit>("localhost:8765");
	});
}

std::unique_ptr<SceneControls>
Scenario_DrosophilaRegular::provideSceneControls() const {
	return std::make_unique<SceneControls>(DefaultSceneControls);
}
