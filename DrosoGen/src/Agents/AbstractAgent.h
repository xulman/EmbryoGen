#ifndef ABSTRACTAGENT_H
#define ABSTRACTAGENT_H

#include <i3d/image3d.h>
#include "../DisplayUnits/DisplayUnit.h"
#include "../Geometries/Geometry.h"
class Simulation;

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
	ShadowAgent(Geometry& geom, const std::string& type)
		: geometry(geom), agentType(type) {};

	/** The geometry of an agent that is exposed to the world.
	    It might be a light-weight version of the agent's exact geometry.
	    However, it is this geometry that is examined for calculating
	    distances between agents. See also the discussion AbstractAgent::drawMask(). */
	Geometry& geometry;

	/** The type designation of this agent (that is represented with this->geometry).
	    Simulation agents may decide to "pay attention to"/smell/interact with only
	    certain types of agents and this attribute is a way to identify/distinguish
	    between various types. The type identifier can be arbitrary, there is no nomenclature
	    nor format (technically) enforced.

	    Notice that the type can be changed (e.g. when cell goes for Apoptosis, in which case
	    it might want to be registered in the system for a while (to simulate its dissolution)
	    while (by changing its designation) to "communicate" other agents its new "state" and
	    to allow them act accordingly). */
	std::string agentType;

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

	/** returns read-only reference on agent's designation */
	const std::string& getAgentType(void) const
	{
		return agentType;
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
	AbstractAgent(const int _ID, const std::string& _type,
	              Geometry& geometryContainer,
	              const float _currTime, const float _incrTime)
		: ShadowAgent(geometryContainer,_type),
		  currTime(_currTime), incrTime(_incrTime),
		  ID(_ID) {};

public:
	/** Please, override in inherited classes (see docs of AbstractAgent). */
	virtual
	~AbstractAgent() {};

	// ------------- interaction from the Simulation class -------------
	/** (re)sets the officer to which this agent belongs to, this is also
	    a communaction handler back to the Simulation class */
	void setOfficer(Simulation* _officer)
	{
		if (_officer == NULL)
			throw new std::runtime_error("AbstractAgent::setOfficer(): got NULL new Officer.");

		Officer = _officer;
	}

	/** Enables/disables the visual inspection mode of this agent. When enabled,
	    the agent might be drawing differently and likely "more verbosely". */
	void setDetailedDrawingMode(const bool state)
	{
		detailedDrawingMode = state;
	}

	/** Enables/disables the calculations inspection mode of this agent. When enabled,
	    the agent might be reporting its internal states "more verbosely". */
	void setDetailedReportingMode(const bool state)
	{
		detailedReportingMode = state;
	}

protected:
	Simulation* Officer = NULL;
	bool detailedDrawingMode = false;
	bool detailedReportingMode = false;

	// ------------- local time -------------
	/** agent's local time [min] */
	float currTime;

	/** global time increment, agent might need to know it for planning [min] */
	float incrTime;


public:
	// ------------- to implement one round of simulation -------------
	/** reports the agent's current local time */
	float getLocalTime(void) const
	{
		return currTime;
	}

	/** This is where agent's internal affairs happen, e.g., development
	    of agent's state (other simulated sub-objects, texture), movement
	    decision (but not the movement itself!), chemotaxis smelling etc.,
	    the agent signals its will to change shape by creating (internal) forces.

	    The agent is expected to develop its simulation to at least the given
	    futureGlobalTime, which is where the current simulation round will
	    end up. Should the agent be using different local time step than the
	    global one (this->incrTime), this might require no activity from the agent
	    or, the opposite, might require multiple local iterations if the agent
	    uses smaller time step than the global one. In any case, the agent must
	    keep itself in synchrony with the global time.

	    This method implements agent's development (e.g. the process of aging).
	    It should, therefore, be concluded with adjusting the local time.

	    This method must not change agent's exported geometry. The method may,
	    however, consider the (exported) geometries when making its own decisions
	    (e.g., where to position daughters after the division). The method may
	    change its internal/private geometry, despite this is recommended to
	    be implemented in the advanceAndBuildIntForces() method. */
	virtual
	void advanceAndBuildIntForces(const float futureGlobalTime) =0;

	/** This is where agent's shape (geometry) change is implemented as
	    a result of the acting of internal forces. Don't call updateGeometry()
	    as it will be triggered from the outside automatically. */
	virtual
	void adjustGeometryByIntForces(void) =0;

	/** This is where agent's interaction with its surrounding happen,
	    e.g. with other agents, ECM, force fields such as gravity etc.,
	    and shape change is requested by creating (external) forces. */
	virtual
	void collectExtForces(void) =0;

	/** This is where agent's shape (geometry) change is implemented as
	    a result of the acting of external forces. Don't call updateGeometry()
	    as it will be triggered from the outside automatically. */
	virtual
	void adjustGeometryByExtForces(void) =0;

	/** An agent maintains data structure to represent next-time-point geometry. This one is
	    being built in the current round of simulation, the "futureGeometry" of this agent.
	    Additionally, the this.futureGeometry may be much more detailed than this.geometry.
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

	    It is not expected to render the content of this->geometry as this
	    one might be less accurate -- the this->geometry is designed for assessing
	    mutual distances between all neighboring cells and should be a good trade-off
	    between sparse (fast to examine) representation and rich (accurate distances)
	    representation.

	    Besides, the texture rendering/rasterizing methods below also work with data
	    that are in sync with the futureGeometry.

	    When Imagej2/Scenery/Java DisplayUnit is used for the visualization,
	    note then that the displayed graphics primitives have their own IDs and
	    these must obey certain system/format:

	    ID space of graphics primitives: 31 bits

	         lowest 16 bits: ID of the graphics element itself
	    next lowest  1 bit : proper (=0) or debug (=1) element
	    next lowest 14 bits: "identification" of ONE single cell
	         highest 1 bit : not used (sign bit)

	    note: general purpose elements have cell "identification" equal to 0
	          in which case the debug bit is not applied
	    note: there are 4 graphics primitives: points, lines, vectors, meshes */
	virtual
	void drawMask(DisplayUnit&) {};

	/** Should raster the current detailed shape, i.e. the futureGeometry, into
	    the image; may use this->ID or its state somehow for colors.
	    Must take into account image's resolution and offset. */
	virtual
	//template <class MT> //MT = Mask Type
	void drawMask(i3d::Image3d<i3d::GRAY16>&) {};

	/** Should render the current texture into the DisplayUnit,
	    see AbstractAgent::drawMask(DisplayUnit&) for details. */
	virtual
	void drawTexture(DisplayUnit&) {};

	/** Should raster the current texture and optical properties into the images.
	    Must take into account image's resolution and offset. */
	virtual
	//template <class VT> //VT = Voxel Type
	void drawTexture(i3d::Image3d<float>&, i3d::Image3d<float>&) {};

	/** Render whatever might be appropriate for debug into the DisplayUnit,
	    see AbstractAgent::drawMask(DisplayUnit&) for details. */
	virtual
	void drawForDebug(DisplayUnit&) {};

	/** Raster whatever might be appropriate for debug into the image.
	    Must take into account image's resolution and offset. */
	virtual
	//template <class T> //T = just some Type
	void drawForDebug(i3d::Image3d<i3d::GRAY16>&) {};
};
#endif
