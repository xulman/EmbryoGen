#pragma once

#include "../Geomtery.hpp"
#include "../Mesh.hpp"
#include "../ScalarImg.hpp"
#include "../Spheres.hpp"
#include "../VectorImg.hpp"

Geometry* geometryCreateAndDeserializeFrom(ListOfShapeForms g_type,
                                           char* buffer) {
	switch (g_type) {
	case Geometry::ListOfShapeForms::Spheres:
		return Spheres::createAndDeserializeFrom(buffer);
	case Geometry::ListOfShapeForms::Mesh:
		return Mesh::createAndDeserializeFrom(buffer);
	case Geometry::ListOfShapeForms::ScalarImg:
		return ScalarImg::createAndDeserializeFrom(buffer);
	case Geometry::ListOfShapeForms::VectorImg:
		return VectorImg::createAndDeserializeFrom(buffer);
	case Geometry::ListOfShapeForms::undefGeometry:
	default:
		return NULL;
	}
}
