#ifndef ABSTRACTAGENT_H
#define ABSTRACTAGENT_H

#include <i3d/image3d.h>
#include "../DisplayUnits/DisplayUnit.h"
#include "../Geometries/Geometry.h"

/**
 * This class is essentially only a read-only representation of
 * a geometry/shape a simulated agent.
 *
 * It is typically valid for the last complete simulation global time.
 * Other agents can make their decision making based on this shape, and
 * develop their internal geometry; once the simulation round is over,
 * all agents typically promote their current (new) shape into this one.
 *
 * Author: Vladimir Ulman, 2018
 */
class ShadowAgent
{
protected:
	//TODO add docs
	ShadowAgent(Geometry& geom) : geometry(geom) {};

	/** The geometry of an agent that is exposed to the world.
	    It might be a light-weight version of the agent's exact geometry.
	    However, it is this geometry that is examined for calculating
	    distances between agents. */
	Geometry& geometry;


public:
	/// returns read-only reference to the agent's (axis aligned) bounding box
	const AxisAlignedBoundingBox& getAABB(void) const
	{
		return geometry.AABB;
	}

	/// returns read-only reference to the agent's geometry
	const Geometry& getGeometry(void) const
	{
		return geometry;
	}
};


/**
 * This class is essentially only a collection of (empty) functions
 * that any simulated agent (actor in the simulation that need not
 * necessarily be visible) should implement.
 *
 * Author: Vladimir Ulman, 2018
 */
class AbstractAgent: public ShadowAgent
{
protected:
	//TODO add docs
	AbstractAgent(const int _ID,
	              Geometry& geometryContainer)
		: ShadowAgent(geometryContainer), ID(_ID) {};

public:
	virtual
	~AbstractAgent() {};

protected:
	// ------------- geometry -------------
	/** An agent maintains yet another geometry data structure. This one is being
	    built in the current round of simulation, the "futureGeometry" of this agent.
	    Additionally, the futureGeometry may be much more detailed than this.geometry.
	    However, since this.geometry is what is visible to the outside world, we need
	    a convertor function to "publish" the new geometry to the world. In other words,
	    to update the (old) this.geometry with the (new) this.futureGeometry. */
	virtual
	void updateGeometry(void) =0;


	// ------------- local time -------------
	///agent's local time
	float currTime; //[min]

	///global time increment, agent needs it for planning
	float incrTime; //[min]


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


	// ------------- rendering -------------
	const int ID;

	virtual
	void drawMask(DisplayUnit& du) {};

	virtual
	//template <class MT> //MT = Mask Type
	void drawMask(i3d::Image3d<i3d::GRAY16>& img) {};

	virtual
	void drawTexture(DisplayUnit& du) {};

	virtual
	//template <class VT> //VT = Voxel Type
	void drawTexture(i3d::Image3d<i3d::GRAY16>& img) {};

	virtual
	void drawForDebug(DisplayUnit& du) {};

	virtual
	//template <class T> //T = just some Type
	void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) {};
};
#endif
