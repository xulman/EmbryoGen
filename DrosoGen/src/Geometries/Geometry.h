#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <list>
#include <i3d/image3d.h>
#include "../util/Vector3d.h"
#include "../DisplayUnits/DisplayUnit.h"

/** accuracy of the geometry representation, choose float or double */
#define FLOAT float

/** a coordinate value that is way far outside of any scene... [micrometers] */
#define TOOFAR 999999999

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

	/** construct a copy of the given 'aabb' */
	AxisAlignedBoundingBox(const AxisAlignedBoundingBox& aabb)
		: minCorner( aabb.minCorner ),
		  maxCorner( aabb.maxCorner )
	{}

	/** construct an AABB that represents the entire 'img'*/
	template <typename T>
	AxisAlignedBoundingBox(const i3d::Image3d<T>& img)
		: minCorner( img.GetOffset() ),
		  maxCorner( Vector3d<size_t>(img.GetSize()).to<FLOAT>() )
	{
		maxCorner.toMicrons(Vector3d<FLOAT>(img.GetResolution().GetRes()),minCorner);
	}

	/** adjusts the image's resolution, offset and size to represent
	    this AABB extended with the optional outer frame of the given
	    pixel width */
	template <typename T>
	void adaptImage(i3d::Image3d<T>& img,
	                const Vector3d<float>& res,                                //px/um
	                const Vector3d<short>& pxFrameWidth = Vector3d<short>(2))  //px
	const
	{
		const Vector3d<float> umFrameWidth( Vector3d<float>().from(pxFrameWidth).elemDivBy(res) );

		Vector3d<float> tmp(minCorner.to<float>());
		tmp -= umFrameWidth;

		img.SetResolution( i3d::Resolution(res.toI3dVector3d()) );
		img.SetOffset( tmp.toI3dVector3d() );

		tmp.from(maxCorner-minCorner);
		tmp += 2.0f * umFrameWidth;
		tmp.elemMult(res).elemCeil();
		img.MakeRoom( tmp.to<size_t>().toI3dVector3d() );

		DEBUG_REPORT("from AABB: minCorner=" << minCorner << " --> maxCorner=" << maxCorner);
		DEBUG_REPORT(" to image: imgOffset=" << img.GetOffset() << ", imgSize="
		             << img.GetSize() << ", imgRes=" << res);
	}

	/** exports this AABB as a "sweeping" box that is given
	    with the output 'minSweep' and 'maxSweep' corners and
	    that is appropriate for (that is, intersected with)
	    the given 'img'

	    sweep as x=minSweep; while (x < maxSweep)...,
	    note the '<' (and not '<=') stop criterion */
	template <typename T>
	void exportInPixelCoords(const i3d::Image3d<T>& img,
	                         Vector3d<size_t>& minSweep,
	                         Vector3d<size_t>& maxSweep) const
	{
		//p is minCorner in img's px coordinates
		Vector3d<FLOAT> p(minCorner);
		p.toPixels(img.GetResolution().GetRes(),img.GetOffset());

		//make sure minCorner is within the image
		p.elemMax(Vector3d<FLOAT>(0));
		p.elemMin(Vector3d<FLOAT>(img.GetSize()));

		//obtain integer px coordinate that is already intersected with image dimensions
		minSweep.toPixels(p);

		//p is maxCorner in img's px coordinates
		p = maxCorner;
		p.toPixels(img.GetResolution().GetRes(),img.GetOffset());

		//make sure minCorner is within the image
		p.elemMax(Vector3d<FLOAT>(0));
		p.elemMin(Vector3d<FLOAT>(img.GetSize()));

		//obtain integer px coordinate that is already intersected with image dimensions
		maxSweep.toPixels(p);
	}


	/** resets AABB (make it ready for someone to start filling it) */
	void inline reset(void)
	{
		minCorner = +TOOFAR;
		maxCorner = -TOOFAR;
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


	/** Uses AxisAlignedBoundingBox::drawBox() to render this bounding box. */
	int drawIt(const int ID, const int color, DisplayUnit& du)
	{
		return drawBox(ID,color, minCorner,maxCorner, du);
	}

	/** Renders given bounding box into the given DisplayUnit 'du'
	    under the given 'ID' with the given 'color'. Multiple graphics
	    elements are required, so a couple of consecutive IDs are used
	    starting from the given 'ID'. Returned value tells how many
	    elements were finally used. */
	int drawBox(const int ID, const int color,
	            const Vector3d<FLOAT> &minC,
	            const Vector3d<FLOAT> &maxC,
	            DisplayUnit& du)
	{
		//horizontal lines
		du.DrawLine( ID+0,
		  minC,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  minC.z),color );

		du.DrawLine( ID+1,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  minC.z),
		  Vector3d<float>(maxC.x,
		                  maxC.y,
		                  minC.z),color );

		du.DrawLine( ID+2,
		  Vector3d<float>(minC.x,
		                  minC.y,
		                  maxC.z),
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  maxC.z),color );

		du.DrawLine( ID+3,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  maxC.z),
		  maxC,color );

		//vertical lines
		du.DrawLine( ID+4,
		  minC,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  minC.z),color );

		du.DrawLine( ID+5,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  minC.z),
		  Vector3d<float>(maxC.x,
		                  maxC.y,
		                  minC.z),color );

		du.DrawLine( ID+6,
		  Vector3d<float>(minC.x,
		                  minC.y,
		                  maxC.z),
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  maxC.z),color );

		du.DrawLine( ID+7,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  maxC.z),
		  maxC,color );

		//"axial" lines
		du.DrawLine( ID+8,
		  minC,
		  Vector3d<float>(minC.x,
		                  minC.y,
		                  maxC.z),color );

		du.DrawLine( ID+9,
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  minC.z),
		  Vector3d<float>(maxC.x,
		                  minC.y,
		                  maxC.z),color );

		du.DrawLine( ID+10,
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  minC.z),
		  Vector3d<float>(minC.x,
		                  maxC.y,
		                  maxC.z),color );

		du.DrawLine( ID+11,
		  Vector3d<float>(maxC.x,
		                  maxC.y,
		                  minC.z),
		  maxC,color );

		return 12;
	}
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
	Vector3d<FLOAT> localPos;
	/** position of 'other' colliding point [micrometers] */
	Vector3d<FLOAT> otherPos;

	/** Distance between the localPos and otherPos [micrometers].
	    If the value is negative, the two points represent a collision pair.
	    If the value is positive, the two points are assumed to form a pair
	    of two nearest points between the two geometries. */
	FLOAT distance;

	/** hinting data about the 'local' point */
	long localHint;
	/** hinting data about the 'other' point */
	long otherHint;

	/** Extra data that a caller/user may want to associate with this pair.
	    The caller must take care of this pointer when this pair is destructed. */
	void* callerHint;

	/** convenience constructor for just two colliding points */
	ProximityPair(const Vector3d<FLOAT>& l, const Vector3d<FLOAT>& o,
	              const FLOAT dist)
		: localPos(l), otherPos(o), distance(dist), localHint(0), otherHint(0), callerHint(NULL) {};

	/** convenience constructor for points with hints */
	ProximityPair(const Vector3d<FLOAT>& l, const Vector3d<FLOAT>& o,
	              const FLOAT dist,
	              const long lh, const long oh)
		: localPos(l), otherPos(o), distance(dist), localHint(lh), otherHint(oh), callerHint(NULL) {};

	/** swap the notion of 'local' and 'other' */
	void swap(void)
	{
		Vector3d<FLOAT> tmpV(localPos);
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
	    between myself and some other agent. A (scaled) ForceVector<FLOAT>
	    can be easily constructed from the points of the ProximityPair.
	    The discovered ProximityPairs are added to the current list l. */
	virtual
	void getDistance(const Geometry& otherGeometry,
	                 std::list<ProximityPair>& l) const =0;

	/** Calculate and determine proximity and collision pairs, if any,
	    between myself and some other agent. To facilitate construction
	    of a (scaled) ForceVector<FLOAT> from the proximity pair, a caller
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
	}
};
#endif
