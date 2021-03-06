#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <list>
#include <i3d/image3d.h>
#include "../util/Vector3d.h"
#include "../DisplayUnits/util/RenderingFunctions.h"
class DisplayUnit;

/** accuracy of the geometry representation, choose float or double */
#define G_FLOAT float

/** a coordinate value that is way far outside of any scene... [micrometers] */
#define TOOFAR 999999999.f

/** An x,y,z-axes aligned 3D bounding box for approximate representation of agent's geometry */
class AxisAlignedBoundingBox
{
public:
	/** defines the bounding box with its volumetric diagonal,
	    this is the "bottom-left" corner of the box [micrometers] */
	Vector3d<G_FLOAT> minCorner;

	/** defines the bounding box with its volumetric diagonal,
	    this is the "upper-right" corner of the box [micrometers] */
	Vector3d<G_FLOAT> maxCorner;

	/** construct an empty AABB */
	AxisAlignedBoundingBox(void)
	{
		reset();
	}

	/** construct a copy of the given 'aabb' */
	AxisAlignedBoundingBox(const AxisAlignedBoundingBox& aabb)
		: minCorner( aabb.minCorner ),
		  maxCorner( aabb.maxCorner )
	{}

	/** construct an AABB that represents the entire 'img'*/
	template <typename T>
	AxisAlignedBoundingBox(const i3d::Image3d<T>& img)
		: minCorner( img.GetOffset() ),
		  maxCorner( Vector3d<size_t>(img.GetSize()).to<G_FLOAT>() )
	{
		maxCorner.toMicrons(Vector3d<G_FLOAT>(img.GetResolution().GetRes()),minCorner);
	}

	/** adjusts the image's resolution, offset and size to represent
	    this AABB extended with the optional outer frame of the given
	    pixel width */
	template <typename T>
	void adaptImage(i3d::Image3d<T>& img,
	                const Vector3d<float>& res,                                      //px/um
	                const Vector3d<short>& pxFrameWidth = Vector3d<short>(2)) const; //px

	/** exports this AABB as a "sweeping" box that is given
	    with the output 'minSweep' and 'maxSweep' corners and
	    that is appropriate for (that is, intersected with)
	    the given 'img'

	    sweep as x=minSweep; while (x < maxSweep)...,
	    note the '<' (and not '<=') stop criterion */
	template <typename T>
	void exportInPixelCoords(const i3d::Image3d<T>& img,
	                         Vector3d<size_t>& minSweep,
	                         Vector3d<size_t>& maxSweep) const;

	/** resets AABB (make it ready for someone to start filling it) */
	void inline reset(void)
	{
		minCorner = +TOOFAR;
		maxCorner = -TOOFAR;
	}


	/** returns SQUARED shortest distance along any axis between this and
	    the given AABB, or 0.0 if they intersect */
	G_FLOAT minDistance(const AxisAlignedBoundingBox& AABB) const;


	/** Uses RenderingFunctions::drawBox() to render this bounding box. */
	int drawIt(const int ID, const int color, DisplayUnit& du)
	{ return RenderingFunctions::drawBox(du, ID,color, minCorner,maxCorner); }
};


/**
 * An x,y,z-axes aligned 3D bounding box for approximate representation of agent's geometry,
 * the box has an extra (agent)'ID' and (agent)'nameID' attributes that are expected to match
 * the ID and name (ShadowAgent::agentType.getHash()) of the agent this box is representing.
 *
 * The existence of this class comes from the optimization background. Agents in the simulation
 * are free to choose with whom (with which other agents) they want to interact, that is,
 * whose detailed geometry (ShadowAgent's data) they want to consider. Previously,
 * the FO always fetched (FrontOfficer::getNearbyAgents()) ShadowAgent objects of all
 * nearby agents and the agent might have just skipped over/ignored some (and the decision
 * was typically based on the content of ShadowAgent::agentType). This, however, required
 * all nearby ShadowAgent's to be availalbe (thus transferred!). This is no longer necessary
 * with the new pair of methods FrontOfficer::getNearbyAABBs() and FrontOfficer::getNearbyAgent().
 * Here, the agent decides whom to consider based on (the proximity and) the name of the AABB
 * and only then requests the ShadowAgent of the agents that this one is interested in.
 * Typically, 'nameID' is translated back into full agent's name (with the help of local
 * agentsTypesDictionary), and then considered to make decision if the corresponding agent is
 * of interest. The 'ID' is used to properly (and uniquely) map back on the represented agent.
 *
 * Consider also the docs of FrontOfficer::translateNameIdToAgentName() to understand
 * the grand scheme of things, please.
 *
 * Author: Vladimir Ulman, 2019
 */
class NamedAxisAlignedBoundingBox: public AxisAlignedBoundingBox
{
public:
	int ID;
	size_t nameID;

	//mostly repetition of the AxisAlignedBoundingBox c'tors
	NamedAxisAlignedBoundingBox(void)
		: AxisAlignedBoundingBox(), ID(-1), nameID(-1)
	{}

	NamedAxisAlignedBoundingBox(const NamedAxisAlignedBoundingBox& naabb)
		: AxisAlignedBoundingBox(naabb), ID(naabb.ID), nameID(naabb.nameID)
	{}

	NamedAxisAlignedBoundingBox(const AxisAlignedBoundingBox& aabb,
	                            const int boxID, const size_t boxNameID)
		: AxisAlignedBoundingBox(aabb), ID(boxID), nameID(boxNameID)
	{}

	template <typename T>
	NamedAxisAlignedBoundingBox(const i3d::Image3d<T>& img,
	                            const int boxID, const size_t boxNameID)
		: AxisAlignedBoundingBox(img), ID(boxID), nameID(boxNameID)
	{}
};


/**
 * Representation of a nearby pair of points, an output of Geometry::getDistance().
 * The geometry object on which getDistance() is called plays the role of 'local' and
 * the argument of the method plays the role of 'other' in the nomenclature of attribute
 * names. If the distance between the two points is negative, the points represent
 * a colliding pair, and absolute value of the distance represents a "penetration depth".
 *
 * The structure can possibly hold 'helper data' that depend on form of the geometry that
 * is investigated during the getDistance(). This can be, e.g., a pointer on mesh vertex
 * that is in collision, or centre of a sphere that is in collision.
 */
struct ProximityPair
{
	/** position of 'local' colliding point [micrometers] */
	Vector3d<G_FLOAT> localPos;
	/** position of 'other' colliding point [micrometers] */
	Vector3d<G_FLOAT> otherPos;

	/** Distance between the localPos and otherPos [micrometers].
	    If the value is negative, the two points represent a collision pair.
	    If the value is positive, the two points are assumed to form a pair
	    of two nearest points between the two geometries. */
	G_FLOAT distance;

	/** hinting data about the 'local' point */
	long localHint;
	/** hinting data about the 'other' point */
	long otherHint;

	/** Extra data that a caller/user may want to associate with this pair.
	    The caller must take care of this pointer when this pair is destructed. */
	void* callerHint;

	/** convenience constructor for just two colliding points */
	ProximityPair(const Vector3d<G_FLOAT>& l, const Vector3d<G_FLOAT>& o,
	              const G_FLOAT dist)
		: localPos(l), otherPos(o), distance(dist), localHint(0), otherHint(0), callerHint(NULL) {};

	/** convenience constructor for points with hints */
	ProximityPair(const Vector3d<G_FLOAT>& l, const Vector3d<G_FLOAT>& o,
	              const G_FLOAT dist,
	              const long lh, const long oh)
		: localPos(l), otherPos(o), distance(dist), localHint(lh), otherHint(oh), callerHint(NULL) {};

	/** swap the notion of 'local' and 'other' */
	void swap(void)
	{
		Vector3d<G_FLOAT> tmpV(localPos);
		localPos = otherPos;
		otherPos = tmpV;

		long tmpH = localHint;
		localHint = otherHint;
		otherHint = tmpH;
	}
};


/** A common ancestor class/type for the Spheres, Mesh and scalar or vector image
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
		ScalarImg=2,
		VectorImg=3,
		undefGeometry=10
	} ListOfShapeForms;

	/** Construct empty geometry object of given shape form.
	    This class should never be used constructed directly, always use some
	    derived class such as Spheres, Mesh, ScalarImg or VectorImg. */
	Geometry(const ListOfShapeForms _shapeForm) : shapeForm(_shapeForm), AABB() {};


public:
	/** choosen form of shape representation */
	const ListOfShapeForms shapeForm;

	/** optional, i.e. not automatically updated with every geometry change,
	    bounding box to outline where the agent lives in the scene */
	AxisAlignedBoundingBox AABB;

	/** Calculate and determine proximity and collision pairs, if any,
	    between myself and some other agent. A (scaled) ForceVector<G_FLOAT>
	    can be easily constructed from the points of the ProximityPair.
	    The discovered ProximityPairs are added to the current list l. */
	virtual
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const =0;

	/** Calculate and determine proximity and collision pairs, if any,
	    between myself and some other agent. To facilitate construction
	    of a (scaled) ForceVector<G_FLOAT> from the proximity pair, a caller
	    may supply its own callerHint data. This data will be stored in
	    ProximityPair::callerHint only in the newly added ProximityPairs.
	    The discovered ProximityPairs are added to the current list l. */
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l,
	                 void* const callerHint) const
	{
		//remember the length of the input list
		size_t itemsOnTheList = l.size();

		//call the original implementation (that is without callerHint)
		getDistance(otherGeometry,l);

		//scan the newly added items and supply them with the callerHint
		std::list<ProximityPair>::iterator ll = l.end();
		while (itemsOnTheList < l.size())
		{
			--ll;
			ll->callerHint = callerHint;
			++itemsOnTheList;
		}
	}

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
	/** updates the given (foreign) AABB to reflect the current geometry */
	virtual
	void updateThisAABB(AxisAlignedBoundingBox& AABB) const =0;

	/** updates this object's own AABB to reflect the current geometry */
	void updateOwnAABB(void)
	{
		updateThisAABB(this->AABB);
		++version;
	}

	/** a version of this geometry that gets automatically incremented with
	    every call to this->updateOwnAABB(); intended use was that a user remembers
	    the last value of this attribute, and she can later see (by comparison) if
	    the geometry has changed or not.... */
	int version = 0;

	// ----------------- support for serialization and deserealization -----------------
	int getType() const
	{ return (int)shapeForm; }

	virtual long getSizeInBytes() const =0;

	virtual void serializeTo(char* buffer) const =0;
	virtual void deserializeFrom(char* buffer) =0;
	
	static Geometry * createAndDeserializeFrom(/*ListOfShapeForms*/ int g_type, char * buffer);

	// ----------------- support for rasterization -----------------
	virtual void renderIntoMask(i3d::Image3d<i3d::GRAY16>& mask, const i3d::GRAY16 drawID) const =0;
};
#endif
