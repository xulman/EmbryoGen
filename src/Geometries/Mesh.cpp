#include "../util/report.h"
#include "Mesh.h"

/** calculate min surface distance between myself and some foreign agent */
void Mesh::getDistance(const Geometry& otherGeometry,
                       std::list<ProximityPair>& l) const
{
	switch (otherGeometry.shapeForm)
	{
	case ListOfShapeForms::Spheres:
		//TODO: attempt to project mesh vertices against the spheres (or their AABB) and look for collision
		REPORT("this.Mesh vs Spheres is not implemented yet!");
		break;
	case ListOfShapeForms::Mesh:
		//TODO identity case
		REPORT("this.Mesh vs Mesh is not implemented yet!");
		break;
	case ListOfShapeForms::ScalarImg:
	case ListOfShapeForms::VectorImg:
		//find collision "from the other side"
		getSymmetricDistance(otherGeometry,l);
		break;

	case ListOfShapeForms::undefGeometry:
		REPORT("Ignoring other geometry of type 'undefGeometry'.");
		break;
	default:
		throw ERROR_REPORT("Not supported combination of shape representations.");
	}
}


/** construct AABB from the given Mesh */
void Mesh::updateThisAABB(AxisAlignedBoundingBox& /* AABB */) const
{
	//scan through the mesh vertices/nodes and find extremal coordinates
	//TODO
	REPORT("not implemented yet!");
}


long Mesh::getSizeInBytes() const
{
	REPORT("NOT SUPPORTED YET!");
	return 1;
}
