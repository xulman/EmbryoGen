#ifndef AGENTS_NUCLEUSAGENT_H
#define AGENTS_NUCLEUSAGENT_H

#include <list>
#include <vector>
#include "../util/report.h"
#include "AbstractAgent.h"
#include "../Geometries/Spheres.h"

static const ForceName ftype_s2s       = "sphere-sphere";     //internal forces
static const ForceName ftype_drive     = "desired movement";
static const ForceName ftype_friction  = "friction";

static const ForceName ftype_repulsive = "repulsive";         //due to external events with nuclei
static const ForceName ftype_body      = "no overlap (body)";
static const ForceName ftype_slide     = "no sliding";

static const ForceName ftype_hinter    = "sphere-hinter";     //due to external events with shape hinters

static const FLOAT fstrength_body_scale     = (FLOAT)0.4;     // [N/um]      TRAgen: N/A
static const FLOAT fstrength_overlap_scale  = (FLOAT)0.2;     // [N/um]      TRAgen: k
static const FLOAT fstrength_overlap_level  = (FLOAT)0.1;     // [N]         TRAgen: A
static const FLOAT fstrength_overlap_depth  = (FLOAT)0.5;     // [um]        TRAgen: delta_o (do)
static const FLOAT fstrength_rep_scale      = (FLOAT)0.6;     // [1/um]      TRAgen: B
static const FLOAT fstrength_slide_scale    = (FLOAT)1.0;     // unitless
static const FLOAT fstrength_hinter_scale   = (FLOAT)0.25;    // [1/um^2]

/** Color coding:
 0 - white
 1 - red
 2 - green
 3 - blue
 4 - cyan
 5 - magenta
 6 - yellow
 */


class NucleusAgent: public AbstractAgent
{
public:
	NucleusAgent(const int _ID, const std::string& _type,
	             const Spheres& shape,
	             const float _currTime, const float _incrTime)
		: AbstractAgent(_ID,_type, geometryAlias, _currTime,_incrTime),
		  geometryAlias(shape),
		  futureGeometry(shape),
		  accels(new Vector3d<FLOAT>[2*shape.noOfSpheres]),
		  //NB: relies on the fact that geometryAlias.noOfSpheres == futureGeometry.noOfSpheres
		  //NB: accels[] and velocities[] together form one buffer (cache friendlier)
		  velocities(accels+shape.noOfSpheres),
		  weights(new FLOAT[shape.noOfSpheres])
	{
		//update AABBs
		geometryAlias.Geometry::updateOwnAABB();
		futureGeometry.Geometry::updateOwnAABB();

		//estimate of number of forces (per simulation round):
		//10(all s2s) + 4(spheres)*2(drive&friction) + 10(neigs)*4(spheres)*4("outer" forces),
		//and "up-rounded"...
		forces.reserve(200);
		velocity_CurrentlyDesired = 0; //no own movement desired yet
		velocity_PersistenceTime  = (FLOAT)2.0;

		for (int i=0; i < shape.noOfSpheres; ++i) weights[i] = (FLOAT)1.0;

		//DEBUG_REPORT("Nucleus with ID=" << ID << " was just created");
	}

	~NucleusAgent(void)
	{
		delete[] accels; //NB: deletes also velocities[], see above
		delete[] weights;

		//DEBUG_REPORT("Nucleus with ID=" << ID << " was just deleted");
	}


protected:
	// ------------- internals state -------------
	/** motion: desired current velocity [um/min] */
	Vector3d<FLOAT> velocity_CurrentlyDesired;

	/** motion: adaptation time, that is, how fast the desired velocity
	    should be reached (from zero movement); this param is in
	    the original literature termed as persistence time and so
	    we keep to that term [min] */
	FLOAT velocity_PersistenceTime;

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	Spheres geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry, even the same noOfSpheres */
	Spheres futureGeometry;

	/** width of the "retention zone" around nuclei that another nuclei
	    shall not enter; this zone simulates cytoplasm around the nucleus;
	    it actually behaves as if nuclei spheres were this much larger
		 in their radii; the value is in microns */
	float cytoplasmWidth = 2.0f;

	// ------------- externals geometry -------------
	/** limiting distance beyond which I consider no interaction possible
	    with other nuclei */
	float ignoreDistance = 10.0f;

	/** locations of possible interaction with nearby nuclei */
	std::list<ProximityPair> proximityPairs_toNuclei;

	/** locations of possible interaction with nearby yolk */
	std::list<ProximityPair> proximityPairs_toYolk;

	/** locations of possible interaction with guiding trajectories */
	std::list<ProximityPair> proximityPairs_tracks;

	// ------------- forces & movement (physics) -------------
	/** all forces that are in present acting on this agent */
	std::vector< ForceVector3d<FLOAT> > forces;

	/** an aux array of acceleration vectors calculated for every sphere, the length
	    of this array must match the length of the spheres in the 'futureGeometry' */
	Vector3d<FLOAT>* const accels;

	/** an array of velocities vectors of the spheres, the length of this array must match
	    the length of the spheres that are exposed (geometryAlias) to the outer world */
	Vector3d<FLOAT>* const velocities;

	/** an aux array of weights of the spheres, the length of this array must match
	    the length of the spheres in the 'futureGeometry' */
	FLOAT* const weights;

	/** essentially creates a new version (next iteration) of 'futureGeometry' given
	    the current content of the 'forces'; note that, in this particular agent type,
	    the 'geometryAlias' is kept synchronized with the 'futureGeometry' so they seem
	    to be interchangeable, but in general setting the 'futureGeometry' might be more
	    rich representation of the current geometry that is regularly "exported" via publishGeometry()
	    and for which the list of ProximityPairs was built during collectExtForces() */
	void adjustGeometryByForces(void);

	/** helper method to (correctly) create a force acting on a sphere */
	inline
	void exertForceOnSphere(const int sphereIdx,
	                        const Vector3d<FLOAT>& forceVector,
	                        const ForceName forceType)
	{
		forces.emplace_back( forceVector, futureGeometry.centres[sphereIdx],sphereIdx, forceType);
	}

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float futureGlobalTime) override;

	void adjustGeometryByIntForces(void) override
	{
		adjustGeometryByForces();
	}


	void collectExtForces(void) override;

	void adjustGeometryByExtForces(void) override
	{
		adjustGeometryByForces();
	}


	void publishGeometry(void) override
	{
		//promote my NucleusAgent::futureGeometry to my ShadowAgent::geometry, which happens
		//to be overlaid/mapped-over with NucleusAgent::geometryAlias (see the constructor)
		for (int i=0; i < geometryAlias.noOfSpheres; ++i)
		{
			geometryAlias.centres[i] = futureGeometry.centres[i];
			geometryAlias.radii[i]   = futureGeometry.radii[i] +cytoplasmWidth;
		}

		//update AABB
		geometryAlias.Geometry::updateOwnAABB();
	}


public:
	const Vector3d<FLOAT>& getVelocityOfSphere(const long index) const
	{
#ifdef DEBUG
		if (index >= geometryAlias.noOfSpheres)
			throw ERROR_REPORT("requested sphere index out of bound.");
#endif

		return velocities[index];
	}

protected:
	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override;
	void drawForDebug(DisplayUnit& du) override;
	void drawMask(i3d::Image3d<i3d::GRAY16>& img) override;

#ifdef DEBUG
	/** aux memory of the recently generated forces in advanceAndBuildIntForces()
	    and in collectExtForces(), and displayed via drawForDebug() */
	std::vector< ForceVector3d<FLOAT> > forcesForDisplay;
#endif
};
#endif
