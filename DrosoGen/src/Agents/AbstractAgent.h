#ifndef ABSTRACTAGENT_H
#define ABSTRACTAGENT_H

#include <i3d/image3d.h>
#include "../DisplayUnits/DisplayUnit.h"
#include "../Geometries/Geometry.h"

/**
 * This class is essentially only a read-only representation of
 * a geometry/shape of a simulated agent.
 *
 * It is typically valid for the last complete simulation global time.
 * Other agents can make their decision making based on this shape, and
 * develop their own geometries; once the simulation round is over,
 * all agents typically promote their current (new) shape into this one.
 *
 * Author: Vladimir Ulman, 2018
 */
class ShadowAgent
{
protected:
	/** Construct the object (which is an agent shape and position representation)
	    by giving it a concrete implementation of Geometry, e.g. Mesh or Spheres object.
	    The reference to this object is kept and used, i.e. no new object is created. */
	ShadowAgent(Geometry& geom) : geometry(geom) {};

	/** The geometry of an agent that is exposed to the world.
	    It might be a light-weight version of the agent's exact geometry.
	    However, it is this geometry that is examined for calculating
	    distances between agents. See also the discussion AbstractAgent::drawMask(). */
	Geometry& geometry;

public:
	/** returns read-only reference to the agent's (axis aligned) bounding box */
	const AxisAlignedBoundingBox& getAABB(void) const
	{
		return geometry.AABB;
	}

	/** returns read-only reference to the agent's geometry */
	const Geometry& getGeometry(void) const
	{
		return geometry;
	}
};


/**
 * This class is essentially only a collection of (pure virtual) functions
 * that any simulated agent (actor in the simulation that need not
 * necessarily be visible) should implement.
 *
 * It especially hosts the 5 public methods that implement agents own
 * development and consecutive optional shape change, interaction with
 * other agents (purely by means of simulated-physical contact) and again
 * consecutive shape change (also position shape, we use the word 'geometry'
 * to denote both shape and position), and last but not least to exposure
 * of agent's 'futureGeometry' to the current 'geometry' that is maintained
 * in the ancestor class ShadowAgent and that is used for seeing the mutual
 * physical contacts.
 *
 * Any proper simulated actor should inherit from this class and implement
 * its virtual methods (necessarily at least the pure virtual ones).
 *
 * Inheriting class, a new simulation actor, is advised to implement its own
 * geometry not necessarily using the Geometry-derived data structures, as well
 * as other data to store and allow to simulate development of this actor.
 * These should be deleted/freed in the end, which is why the destructor is
 * also virtual in here.
 *
 * Author: Vladimir Ulman, 2018
 */
class AbstractAgent: public ShadowAgent
{
protected:
	/** Define a new agent in the simulation by giving its ID, its current
	    geometry (which get's 'forwarded' to the ShadowAgent), and
	    current global time as well as global time increment. */
	AbstractAgent(const int _ID, Geometry& geometryContainer,
	              const float _currTime, const float _incrTime)
		: ShadowAgent(geometryContainer),
		  currTime(_currTime), incrTime(_incrTime),
		  ID(_ID) {};

public:
	/** Please, override in inherited classes (see docs of AbstractAgent). */
	virtual
	~AbstractAgent() {};

protected:
	// ------------- local time -------------
	/** agent's local time [min] */
	float currTime;

	/** global time increment, agent might need to know it for planning [min] */
	float incrTime;


public:
	// ------------- to implement one round of simulation -------------
	/** This is where agent's internal affairs happen, e.g., development
	    of agent's state (other simulated sub-objects, texture), movement,
	    chemotaxis smelling etc., the agent signals its will to change
	    shape by creating (internal) forces. */
	virtual
	void advanceAndBuildIntForces(void) =0;

	/** This is where agent's shape (geometry) change is implemented as
	    a result of the acting of internal forces. */
	virtual
	void adjustGeometryByIntForces(void) =0;

	/** This is where agent's interaction with its surrounding happen,
	    e.g. with other agents, ECM, force fields such as gravity etc.,
	    and shape changes is requested by creating (external) forces. */
	virtual
	void collectExtForces(void) =0;

	/** This is where agent's shape (geometry) change is implemented as
	    a result of the acting of external forces. This method is typically
	    called at the end of one round of simulation and so it is a good
	    adept for where to call this.updateGeometry(). */
	virtual
	void adjustGeometryByExtForces(void) =0;

	/** An agent maintains data structure to represent next-time-point geometry. This one is
	    being built in the current round of simulation, the "futureGeometry" of this agent.
	    Additionally, the futureGeometry may be much more detailed than this.geometry.
	    However, since this.geometry is what is visible to the outside world, we need
	    a convertor function to "publish" the new geometry to the world. In other words,
	    to update the (old) this.geometry with the (new) this.futureGeometry. */
	virtual
	void updateGeometry(void) =0;


	// ------------- rendering -------------
	/** label of this agent */
	const int ID;

	/** Should render the current detailed shape, i.e. the futureGeometry, into
	    the DisplayUnit; may use this->ID or its state somehow for colors.

	    It is was not expected to render the content of this->geometry as this
	    one might be less accurate -- the this->geometry is designed for assesing
	    mutual distances between all neighboring cells and should be a good trade-off
	    between sparse (fast to examine) representation and rich (accurate distances)
	    representation.

	    Besides, the texture rendering/rasterizing methods below also work with data
	    that are in sync with the futureGeometry. */
	virtual
	void drawMask(DisplayUnit&) {};

	/** Should raster the current detailed shape, i.e. the futureGeometry, into
	    the image; may use this->ID or its state somehow for colors.
	    Must take into account image's resolution and offset. */
	virtual
	//template <class MT> //MT = Mask Type
	void drawMask(i3d::Image3d<i3d::GRAY16>&) {};

	/** Should render the current texture into the DisplayUnit */
	virtual
	void drawTexture(DisplayUnit&) {};

	/** Should raster the current texture into the image.
	    Must take into account image's resolution and offset. */
	virtual
	//template <class VT> //VT = Voxel Type
	void drawTexture(i3d::Image3d<i3d::GRAY16>&) {};

	/** Render whatever might be appropriate for debug into the DisplayUnit. */
	virtual
	void drawForDebug(DisplayUnit&) {};

	/** Raster whatever might be appropriate for debug into the image.
	    Must take into account image's resolution and offset. */
	virtual
	//template <class T> //T = just some Type
	void drawForDebug(i3d::Image3d<i3d::GRAY16>&) {};
};
#endif
