#ifndef NUCLEUSAGENT_H
#define NUCLEUSAGENT_H

#include <list>
#include <vector>
#include "../util/report.h"
#include "AbstractAgent.h"
#include "CellCycle.h"
#include "../Geometries/Spheres.h"

static ForceName ftype_s2s = "sphere-sphere";       //internal forces
static ForceName ftype_drive = "desired movement";
static ForceName ftype_friction = "friction";

static ForceName ftype_repulsive = "repulsive";     //due to external events with nuclei
static ForceName ftype_body = "no overlap (body)";
static ForceName ftype_slide = "no sliding";

static ForceName ftype_hinter = "sphere-hinter";    //due to external events with shape hinters


class NucleusAgent: public AbstractAgent
{
public:
	NucleusAgent(const int ID, const std::string& type,
	             const Spheres& shape,
	             const float currTime, const float incrTime)
		: AbstractAgent(ID,type, geometryAlias, currTime,incrTime),
		  geometryAlias(shape),
		  futureGeometry(shape),
		  velocities(new Vector3d<FLOAT>[2*shape.noOfSpheres]),
		  //NB: relies on the fact that geometryAlias.noOfSpheres == futureGeometry.noOfSpheres
		  //NB: velocities[] and finalForces[] together form one buffer (cache friendlier)
		  finalForces(velocities+shape.noOfSpheres)
	{
		//update AABBs
		geometryAlias.Geometry::setAABB();
		futureGeometry.Geometry::setAABB();
		forces.reserve(30); //NB: 4*7 "up-rounded"

		curPhase = G1Phase;

		DEBUG_REPORT("Nucleus with ID=" << ID << " was just created");
	}

	~NucleusAgent(void)
	{
		delete[] velocities; //NB: deletes also finalForces[], see above

		DEBUG_REPORT("Nucleus with ID=" << ID << " was just deleted");
	}


private:
	// ------------- internals state -------------
	CellCycleParams cellCycle;

	/** currently exhibited cell phase */
	ListOfPhases curPhase;

	// ------------- internals geometry -------------
	/** reference to my exposed geometry ShadowAgents::geometry */
	Spheres geometryAlias;

	/** my internal representation of my geometry, which is exactly
	    of the same form as my ShadowAgent::geometry, even the same noOfSpheres */
	Spheres futureGeometry;

	// ------------- externals geometry -------------
	/** limiting distance beyond which I consider no interaction possible
	    with other nuclei */
	float ignoreDistance = 10.0f;

	/** locations of possible interaction with nearby nuclei */
	std::list<ProximityPair> proximityPairs_toNuclei;

	/** locations of possible interaction with nearby yolk */
	std::list<ProximityPair> proximityPairs_toYolk;

	// ------------- forces & movement -------------
	/** all forces that are in present acting on this agent */
	std::vector< ForceVector3d<FLOAT> > forces;

	/** an array of velocities vectors of the spheres, the length of this array
	    must match the length of the spheres that are exposed to the outer world */
	Vector3d<FLOAT>* const velocities;

public:
	const Vector3d<FLOAT>& getVelocityOfSphere(const int index)
	{
#ifdef DEBUG
		if (index >= geometryAlias.noOfSpheres)
			throw new std::runtime_error("NucleusAgent::getVelocityOfSphere(): requested sphere index out of bound.");
#endif

		return velocities[index];
	}

private:
	/** an aux array of final forces calculated for every sphere, the length of this
	    array must match the length of the spheres in the 'futureGeometry' */
	Vector3d<FLOAT>* const finalForces;

	/** essentially creates a new version (next iteration) of 'futureGeometry' given
	    the current content of the 'forces'; note that, in this particular agent type,
	    the 'geometryAlias' is kept synchronized with the 'futureGeometry' so they seem
	    to be interchangeable, but in general setting the 'futureGeometry' might be more
	    rich representation of the current geometry that is regularly "exported" via updateGeometry()
	    and for which the list of ProximityPairs was built during collectExtForces() */
	void adjustGeometryByForces(void)
	{
		//reset the array with final forces
		for (int i=0; i < futureGeometry.noOfSpheres; ++i) finalForces[i] = 0;

		//collect all forces acting on every sphere to have one overall force per sphere
		for (const auto& f : forces) finalForces[f.hint] += f;

		//now, translation is a result of forces:
		for (int i=0; i < futureGeometry.noOfSpheres; ++i)
		{
			//F=ma -> a=F/m  (volume of a sphere should be taken into account)
			//assumes m=1.0

			//v=at
			velocities[i] += (FLOAT)incrTime * finalForces[i];

			//|trajectory|=vt
			futureGeometry.centres[i] += (FLOAT)incrTime * velocities[i];
		}

		//update AABB to the new geometry
		futureGeometry.Geometry::setAABB();

		//all forces processed...
		forces.clear();
	}

	// ------------- to implement one round of simulation -------------
	void advanceAndBuildIntForces(const float) override
	{
		//check bending of the spheres (how much their position deviates from a line),
		//check also the distances, and create, if necessary, another forces on the list
		/*
		//TODO anti-bending
		Vector3d<FLOAT> refAxis(futureGeometry.centres[2]);
		refAxis -= futureGeometry.centres[1];

		ftype_s2s
		*/

		//create forces that "are product of my will"
		forces.push_back( ForceVector3d<FLOAT>(1.0f,0.0f,0.0f, geometryAlias.centres[1], ftype_drive) );
		forces.back().hint = 1;
		forces.push_back( ForceVector3d<FLOAT>(1.0f,0.0f,0.0f, geometryAlias.centres[2], ftype_drive) );
		forces.back().hint = 2;

		//increase the local time of the agent
		currTime += incrTime;
	}

	void adjustGeometryByIntForces(void) override
	{
		adjustGeometryByForces();
	}

	void collectExtForces(void) override
	{
		//scheduler, please give me ShadowAgents that are not further than ignoreDistance
		//(and the distance is evaluated based on distances of AABBs)
		std::list<const ShadowAgent*> l;
		Officer->getNearbyAgents(this,ignoreDistance,l);

		DEBUG_REPORT("ID " << ID << ": Found " << l.size() << " nearby agents");

		//those on the list are ShadowAgents who are potentially close enough
		//to interact with me and these I need to inspect closely
		proximityPairs_toNuclei.clear();
		proximityPairs_toYolk.clear();
		for (auto sa = l.begin(); sa != l.end(); ++sa)
		{
			if ( ((*sa)->getAgentType())[0] == 'n' )
				geometry.getDistance((*sa)->getGeometry(),proximityPairs_toNuclei);
			else
				geometry.getDistance((*sa)->getGeometry(),proximityPairs_toYolk);
		}

		//now, postprocess the proximityPairs
		DEBUG_REPORT("ID " << ID << ": Found " << proximityPairs_toNuclei.size() << " proximity pairs to nuclei");
		DEBUG_REPORT("ID " << ID << ": Found " << proximityPairs_toYolk.size()   << " proximity pairs to yolk");

		//TODO: ftype_friction as a function of current velocity of spheres

		//convert proximityPairs_toNuclei -> forces! according to TRAgen rules
		//TODO: ftype_repulsive, ftype_body, ftype_slide

		//convert proximityPairs_toYolk -> forces!
		//TODO: ftype_hinter
	}

	void adjustGeometryByExtForces(void) override
	{
		adjustGeometryByForces();
	}

	void updateGeometry(void) override
	{
		//promote my futureGeometry to my geometry, which happens
		//to be overlaid/mapped-over with geometryAlias (see the constructor)
		for (int i=0; i < geometryAlias.noOfSpheres; ++i)
		{
			geometryAlias.centres[i] = futureGeometry.centres[i];
			geometryAlias.radii[i]   = futureGeometry.radii[i];
		}

		//update AABB
		geometryAlias.Geometry::setAABB();
	}

	// ------------- rendering -------------
	void drawMask(DisplayUnit& du) override
	{
		if (ID % 10 != 0) //show only for every 10th cell
			return;

		const int color = curPhase < 3? 2:3;
		int dID = ID << 17;

		//draw spheres
		for (int i=0; i < futureGeometry.noOfSpheres; ++i)
		{
			if (futureGeometry.radii[i] > 0.f)
				du.DrawPoint(dID++,futureGeometry.centres[i],futureGeometry.radii[i],color);
		}

		//draw (debug) vectors
		{
			dID |= 1 << 16; //enable debug bit
			for (auto& p : proximityPairs_toNuclei)
				du.DrawLine(dID++, p.localPos, p.otherPos);
			for (auto& p : proximityPairs_toYolk)
				du.DrawLine(dID++, p.localPos, p.otherPos,1);
		}

		//draw global debug bounding box
		futureGeometry.AABB.drawIt(ID << 4,color,du);
	}

	void drawMask(i3d::Image3d<i3d::GRAY16>& img) override
	{
		//shortcuts to the mask image parameters
		const i3d::Vector3d<float>& res = img.GetResolution().GetRes();
		const Vector3d<FLOAT>       off(img.GetOffset().x,img.GetOffset().y,img.GetOffset().z);

		//shortcuts to our Own spheres
		const Vector3d<FLOAT>* const centresO = futureGeometry.getCentres();
		const FLOAT* const radiiO             = futureGeometry.getRadii();
		const int iO                          = futureGeometry.getNoOfSpheres();

		//project and "clip" this AABB into the img frame
		//so that voxels to sweep can be narrowed down...
		//
		//   sweeping position and boundaries (relevant to the 'img')
		Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
		futureGeometry.AABB.exportInPixelCoords(img, minSweepPX,maxSweepPX);
		//
		//micron coordinate of the running voxel 'curPos'
		Vector3d<FLOAT> centre;

		//sweep and check intersection with spheres' volumes
		for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
		for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
		for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
		{
			//get micron coordinate of the current voxel's centre
			centre.x = ((FLOAT)curPos.x +0.5f) / res.x;
			centre.y = ((FLOAT)curPos.y +0.5f) / res.y;
			centre.z = ((FLOAT)curPos.z +0.5f) / res.z;
			centre += off;

			//check the current voxel against all spheres
			for (int i = 0; i < iO; ++i)
			{
				//if sphere's surface would be 2*lenPXhd thick, would the voxel's center be in?
				if ((centre-centresO[i]).len() <= radiiO[i])
				{
#ifdef DEBUG
					i3d::GRAY16 val = img.GetVoxel(curPos.x,curPos.y,curPos.z);
					if (val > 0 && val != (i3d::GRAY16)ID)
						REPORT(ID << " overwrites mask at " << curPos);
#endif
					img.SetVoxel(curPos.x,curPos.y,curPos.z, (i3d::GRAY16)ID);
				}
			}
		}
	}

	void drawForDebug(i3d::Image3d<i3d::GRAY16>& img) override
	{
		drawMask(img);
	}
};
#endif
