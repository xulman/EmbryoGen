#pragma once

#include "../Geometry.hpp"
#include "../Mesh.hpp"
#include "../ScalarImg.hpp"
#include "../Spheres.hpp"
#include "../VectorImg.hpp"
#include <span>
#include <cstddef>

inline std::unique_ptr<Geometry>
geometryCreateAndDeserialize(Geometry::ListOfShapeForms g_type,
                             std::span<const std::byte> bytes) {
	switch (g_type) {
	case Geometry::ListOfShapeForms::Spheres:
		return std::make_unique<Spheres>(Spheres::createAndDeserialize(bytes));
	case Geometry::ListOfShapeForms::Mesh:
		return std::make_unique<Mesh>(Mesh::createAndDeserialize(bytes));
	case Geometry::ListOfShapeForms::ScalarImg:
		return std::make_unique<ScalarImg>(
		    ScalarImg::createAndDeserialize(bytes));
	case Geometry::ListOfShapeForms::VectorImg:
		return std::make_unique<VectorImg>(
		    VectorImg::createAndDeserialize(bytes));
	case Geometry::ListOfShapeForms::undefGeometry:
	default:
		return nullptr;
	}
}
