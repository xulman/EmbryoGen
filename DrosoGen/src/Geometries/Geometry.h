#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <list>
#include "../util/Vector3d.h"

/** accuracy of the geometry representation, choose float or double */
#define FLOAT float

/** An x,y,z-axes aligned 3D bounding box for approximate representation of agent's geometry */
class AxisAlignedBoundingBox
{
public:
	/** defines the bounding box with its volumetric diagonal,
	    this is the "bottom-left" corner of the box [micrometers] */
	Vector3d<FLOAT> minCorner;

	/** defines the bounding box with its volumetric diagonal,
	    this is the "upper-right" corner of the box [micrometers] */
	Vector3d<FLOAT> maxCorner;

	/** construct an empty AABB */
	AxisAlignedBoundingBox(void)
	{
		reset();
	}


	/** resets AABB (make it ready for someone to start filling it) */
	void inline reset(void)
	{
		minCorner = FLOAT(+999999999.0);
		maxCorner = FLOAT(-999999999.0);
	}


	/** returns SQUARED shortest distance along any axis between this and
	    the given AABB, or 0.0 if they intersect */
	FLOAT minDistance(const AxisAlignedBoundingBox& AABB) const
	{
		FLOAT M = std::max(minCorner.x,AABB.minCorner.x);
		FLOAT m = std::min(maxCorner.x,AABB.maxCorner.x);
		FLOAT dx = M > m ? M-m : 0; //min dist along x-axis

		M = std::max(minCorner.y,AABB.minCorner.y);
		m = std::min(maxCorner.y,AABB.maxCorner.y);
		FLOAT dy = M > m ? M-m : 0; //min dist along y-axis

		M = std::max(minCorner.z,AABB.minCorner.z);
		m = std::min(maxCorner.z,AABB.maxCorner.z);
		FLOAT dz = M > m ? M-m : 0; //min dist along z-axis

		return (dx*dx + dy*dy + dz*dz);
	}
};


/**
 * Representation of a nearby pair of points, an output of Geometry::getDistance().
 * The geometry object on which getDistance() is called plays the role of 'local' and
 * the argument of the method plays the role of 'other' in the nomenclature of attribute
 * names. If the distance between the two points is negative, the points represent
 * a colliding pair.
 *
 * The structure can possibly hold 'helper data' that depend on form of the geometry that
 * is investigated during the getDistance(). This can be, e.g., a pointer on mesh vertex
 * that is in collision, or centre of a sphere that is in collision.
 */
struct ProximityPair
{
	/** position of 'local' colliding point [micrometers] */
	Vector3d<FLOAT> localPos;
	/** position of 'other' colliding point [micrometers] */
	Vector3d<FLOAT> otherPos;

	/** Distance between the localPos and otherPos [micrometers].
	    If the value is negative, the two points represent a collision pair.
	    If the value is positive, the two points are assumed to form a pair
	    of two nearest points between the two geometries. */
	FLOAT distance;

	/** pointer on hinting data of the 'local' point */
	void* localHint;
	/** pointer on hinting data of the 'other' point */
	void* otherHint;

	/** convenience constructor for just two colliding points */
	ProximityPair(const Vector3d<FLOAT>& l, const Vector3d<FLOAT>& o,
	              const FLOAT dist)
		: localPos(l), otherPos(o), distance(dist), localHint(NULL), otherHint(NULL) {};

	/** convenience constructor for points with hints */
	ProximityPair(const Vector3d<FLOAT>& l, const Vector3d<FLOAT>& o,
	              const FLOAT dist,
	              void* lh, void* oh)
		: localPos(l), otherPos(o), distance(dist), localHint(lh), otherHint(oh) {};

	/** swap the notion of 'local' and 'other' */
	void swap(void)
	{
		Vector3d<FLOAT> tmpV(localPos);
		localPos = otherPos;
		otherPos = tmpV;

		void* tmpH = localHint;
		localHint = otherHint;
		otherHint = tmpH;
	}
};


/** A common ancestor class/type for the Spheres, Mesh and Mask Image
    representations of agent's geometry. It defines (pure virtual) methods
    to report collision pairs, see Geometry::getDistance(), between agents. */
class Geometry
{
protected:
	/** A variant of how the shape of an simulated agent is represented */
	typedef enum
	{
		Spheres=0,
		Mesh=1,
		MaskImg=2
	} ListOfShapeForms;

	/** Construct empty geometry object of given shape form.
	    This class should never be used constructed directly, always use some
	    derived class such as Spheres, Mesh or MaskImg. */
	Geometry(const ListOfShapeForms _shapeForm) : shapeForm(_shapeForm), AABB() {};


public:
	/** choosen form of shape representation */
	const ListOfShapeForms shapeForm;

	/** optional, i.e. not automatically updated with every geometry change,
	    bounding box to outline where the agent lives in the scene */
	AxisAlignedBoundingBox AABB;

	/** Calculate and determine proximity and collision pairs, if any,
	    between myself and some other agent. A (scaled) ForceVector<FLOAT>
	    can be easily constructed from the points of the ProximityPair.
	    The discovered ProximityPairs are added to the current list l. */
	virtual
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const =0;

protected:
	/** Helper routine to complement getDistance() with the symmetric cases.
	    For example, to provide measurement between Mesh and Spheres when
	    there is an implementation between Spheres and Mesh. In the symmetric
	    case, the getDistance() is in a reversed/symmetric setting and the
	    ProximityPairs are reversed afterwards (to make 'local' relevant to
	    this object). */
	void getSymmetricDistance(const Geometry& otherGeometry,
	                          std::list<ProximityPair>& l) const
	{
		//setup a new list for the symmetric case...
		std::list<ProximityPair> nl;
		otherGeometry.getDistance(*this,nl);

		//...and reverse ProximityPairs afterwards
		for (auto p = nl.begin(); p != nl.end(); ++p) p->swap();

		//"return" the original list with the new one
		l.splice(l.end(),nl);
	}


public:
	/** sets the given AABB to reflect the current geometry */
	virtual
	void setAABB(AxisAlignedBoundingBox& AABB) const =0;

	/** sets this object's own AABB to reflect the current geometry */
	void setAABB(void)
	{
		setAABB(this->AABB);
	}
};
#endif
