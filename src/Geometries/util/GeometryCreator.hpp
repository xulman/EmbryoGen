#pragma once

#include "../Geometry.hpp"
#include "../Mesh.hpp"
#include "../ScalarImg.hpp"
#include "../Spheres.hpp"
#include "../VectorImg.hpp"

inline std::unique_ptr<Geometry>
geometryCreateAndDeserializeFrom(Geometry::ListOfShapeForms g_type,
                                 const char* buffer) {
	switch (g_type) {
	case Geometry::ListOfShapeForms::Spheres:
		return std::make_unique<Spheres>(
		    Spheres::createAndDeserializeFrom(buffer));
	case Geometry::ListOfShapeForms::Mesh:
		return std::make_unique<Mesh>(Mesh::createAndDeserializeFrom(buffer));
	case Geometry::ListOfShapeForms::ScalarImg:
		return std::make_unique<ScalarImg>(
		    ScalarImg::createAndDeserializeFrom(buffer));
	case Geometry::ListOfShapeForms::VectorImg:
		return std::make_unique<VectorImg>(
		    VectorImg::createAndDeserializeFrom(buffer));
	case Geometry::ListOfShapeForms::undefGeometry:
	default:
		return nullptr;
	}
}
