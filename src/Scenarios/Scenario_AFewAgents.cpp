#include "../Agents/Nucleus4SAgent.hpp"
#include "../Agents/ShapeHinter.hpp"
#include "../DisplayUnits/SceneryBufferedDisplayUnit.hpp"
#include "../Geometries/ScalarImg.hpp"
#include "../Geometries/Spheres.hpp"
#include "../util/Vector3d.hpp"
#include "common/Scenarios.hpp"
#include <memory>

// no specific Nucleus (or other "prefilled" agent)

//==========================================================================
void Scenario_AFewAgents::initializeAgents(FrontOfficer* fo, int p, int) {
	if (p != 1) {
		report::message(fmt::format(
		    "Populating only the first FO (which is not this one)."));
		return;
	}

	const float radius = 0.4f * params->constants.sceneSize.y;
	const int howManyToPlace = 6;

	Vector3d<float> sceneCentre(params->constants.sceneSize);
	sceneCentre /= 2.0f;
	sceneCentre += params->constants.sceneOffset;

	for (int i = 0; i < howManyToPlace; ++i) {
		const float ang = float(i) / float(howManyToPlace);

		// the wished position relative to [0,0,0] centre
		Vector3d<float> pos(radius * std::cos(ang * 6.28f),
		                    radius * std::sin(ang * 6.28f), 0.0f);

		// position is shifted to the scene centre, accounting for scene offset
		pos += sceneCentre;

		Spheres s(4);
		s.updateCentre(0, pos + Vector3d<float>(0, 0, -9));
		s.updateRadius(0, 3.0f);
		s.updateCentre(1, pos + Vector3d<float>(0, 0, -3));
		s.updateRadius(1, 5.0f);
		s.updateCentre(2, pos + Vector3d<float>(0, 0, +3));
		s.updateRadius(2, 5.0f);
		s.updateCentre(3, pos + Vector3d<float>(0, 0, +9));
		s.updateRadius(3, 3.0f);

		fo->startNewAgent(std::make_unique<Nucleus4SAgent>(
		    fo->getNextAvailAgentID(), "nucleus", s, params->constants.initTime,
		    params->constants.incrTime));
	}

	// shadow hinter geometry (micron size and resolution)
	Vector3d<float> size(2.9f * radius, 2.2f * radius, 1.5f * radius);
	const float xRes = 0.8f; // px/um
	const float yRes = 0.8f;
	const float zRes = 0.8f;
	report::debugMessage(
	    fmt::format("Shape hinter image size   [um]: {}", toString(size)));

	// allocate and init memory for the hinter representation
	i3d::Image3d<i3d::GRAY8> Img;
	Img.MakeRoom((size_t)(size.x * xRes), (size_t)(size.y * yRes),
	             (size_t)(size.z * zRes));
	Img.GetVoxelData() = 0;

	// metadata...
	size *= -0.5f;
	size += sceneCentre;
	Img.SetOffset(i3d::Offset(size.x, size.y, size.z));
	Img.SetResolution(i3d::Resolution(xRes, yRes, zRes));
	report::debugMessage(
	    fmt::format("Shape hinter image offset [um]: {}", toString(size)));

	// fill the actual shape (except for a dX x dY x dZ frame at the border)
	const size_t dZ = (size_t)(0.1 * (double)Img.GetSizeZ());
	const size_t dY = (size_t)(0.2 * (double)Img.GetSizeY());
	const size_t dX = (size_t)(0.1 * (double)Img.GetSizeX());
	for (size_t z = dZ; z <= Img.GetSizeZ() - dZ; ++z)
		for (size_t y = dY; y <= Img.GetSizeY() - dY; ++y)
			for (size_t x = dX; x <= Img.GetSizeX() - dX; ++x)
				Img.SetVoxel(x, y, z, 20);
	// Img.SaveImage("GradIN_ZeroOUT__original.tif");

	// now convert the actual shape into the shape geometry
	ScalarImg m(Img, ScalarImg::DistanceModel::GradIN_ZeroOUT);
	// m.saveDistImg("GradIN_ZeroOUT.tif");

	// finally, create the simulation agent to register this shape
	fo->startNewAgent(std::make_unique<ShapeHinter>(fo->getNextAvailAgentID(),
	                                                "yolk", m,
	                                                params->constants.initTime,
	                                                params->constants.incrTime),
	                  false);
}

void Scenario_AFewAgents::initializeScene() {
	displays.registerDisplayUnit([]() {
		return std::make_unique<SceneryBufferedDisplayUnit>("localhost:8765");
	});
}

std::unique_ptr<SceneControls>
Scenario_AFewAgents::provideSceneControls() const {
	return std::make_unique<SceneControls>(DefaultSceneControls);
}
