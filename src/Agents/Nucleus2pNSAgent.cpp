#include <cmath>
#include "Nucleus2pNSAgent.h"

void Nucleus2pNSAgent::updateAuxAxes()
{
	centreAxis = futureGeometry.centres[1] - futureGeometry.centres[0];
	centreAxis.changeToUnitOrZero();
	aux3rdAxis = crossProduct(centreAxis,basalPlaneNormal);
	aux3rdAxis.changeToUnitOrZero();
}


void Nucleus2pNSAgent::defineSpheresLocalCoords()
{
	updateAuxAxes();
	for (size_t i=2; i < (unsigned)futureGeometry.noOfSpheres; ++i)
	{
		spheresLocalCoords[i-2].x = dotProduct( futureGeometry.centres[i]-futureGeometry.centres[0], centreAxis );
		spheresLocalCoords[i-2].y = dotProduct( futureGeometry.centres[i]-futureGeometry.centres[0], basalPlaneNormal );
		spheresLocalCoords[i-2].z = dotProduct( futureGeometry.centres[i]-futureGeometry.centres[0], aux3rdAxis );
	}
}


void Nucleus2pNSAgent::getCurrentOffVectorsForCentres()
{
	updateAuxAxes();
	for (size_t i=2; i < (unsigned)futureGeometry.noOfSpheres; ++i)
	{
		const size_t j = i-2;
		//the expected coordinate
		offs[j].x = spheresLocalCoords[j].x * centreAxis.x
		          + spheresLocalCoords[j].y * basalPlaneNormal.x
		          + spheresLocalCoords[j].z * aux3rdAxis.x;

		offs[j].y = spheresLocalCoords[j].x * centreAxis.y
		          + spheresLocalCoords[j].y * basalPlaneNormal.y
		          + spheresLocalCoords[j].z * aux3rdAxis.y;

		offs[j].z = spheresLocalCoords[j].x * centreAxis.z
		          + spheresLocalCoords[j].y * basalPlaneNormal.z
		          + spheresLocalCoords[j].z * aux3rdAxis.z;
		offs[j] += futureGeometry.centres[0];

		//minus the current coordinate
		offs[j] -= futureGeometry.centres[i];
	}
}


void Nucleus2pNSAgent::advanceAndBuildIntForces(const float futureGlobalTime)
{
	getCurrentOffVectorsForCentres();

	for (int i=2; i < futureGeometry.noOfSpheres; ++i)
		DEBUG_REPORT("sphere #" << i << " delta is " << offs[i-2].len() << " " << offs[i-2]);

	//tolerated mis-position (no "adjustment" s2s forces are created within this radius)
	const FLOAT keepCalmDistance = (FLOAT)0.1;
	const FLOAT keepCalmDistanceSq = keepCalmDistance*keepCalmDistance;

	Vector3d<FLOAT> forceVec;

	//the configuration between the first two spheres
	FLOAT currCentreDistance = (futureGeometry.centres[1] - futureGeometry.centres[0]).len();
	if (std::abs(currCentreDistance - centreDistance) > keepCalmDistance)
	{
		//properly scaled force acting on the 1st sphere: body_scale * len()
		forceVec  = centreAxis; //NB: is of unit length
		forceVec *= (currCentreDistance-centreDistance) * fstrength_body_scale;

		forces.emplace_back( forceVec, futureGeometry.centres[0],0, ftype_s2s );
		distributeCounterForcesAmongTouchingSpheres(forces.back());

		forceVec *= -1.0;
		forces.emplace_back( forceVec, futureGeometry.centres[1],1, ftype_s2s );
		distributeCounterForcesAmongTouchingSpheres(forces.back());
	}

	//the configurations of the remaining spheres
	for (int i=2; i < futureGeometry.noOfSpheres; ++i)
	if (offs[i-2].len2() > keepCalmDistanceSq)
	{
		//properly scaled force: body_scale * len()
		offs[i-2] *= fstrength_body_scale;
		forces.emplace_back( offs[i-2], futureGeometry.centres[i],i, ftype_s2s );
		distributeCounterForcesAmongTouchingSpheres(forces.back());
	}

	NucleusAgent::advanceAndBuildIntForces(futureGlobalTime);
}


void Nucleus2pNSAgent::distributeCounterForcesAmongTouchingSpheres(const ForceVector3d<FLOAT>& counterForce)
{
	const int sourceSphere = (int)counterForce.hint;

	//determine overlaps, and their sum
	FLOAT sum = 0;
	for (int i=0; i < futureGeometry.noOfSpheres; ++i)
	{
		if (i != sourceSphere)
		{
			FLOAT dist = (futureGeometry.centres[i] - futureGeometry.centres[sourceSphere]).len();
			dist -= futureGeometry.radii[i] + futureGeometry.radii[sourceSphere];
			overlaps[i] = std::max(-dist,0.f);
		}
		else overlaps[i] = 0;
		sum += overlaps[i];
	}

	//define forces
	for (int i=0; i < futureGeometry.noOfSpheres; ++i)
	if (overlaps[i] > 0)
		forces.emplace_back( (-overlaps[i]/sum) * counterForce, futureGeometry.centres[i],i, ftype_s2s );
}


void Nucleus2pNSAgent::drawMask(DisplayUnit& du)
{
	int dID  = DisplayUnit::firstIdForAgentObjects(ID);
	int ldID = DisplayUnit::firstIdForAgentDebugObjects(ID);
	int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID*40 +5000;

	//draw spheres, each at different color
	for (int i=0; i < futureGeometry.noOfSpheres; ++i)
		du.DrawPoint(dID++, futureGeometry.centres[i],futureGeometry.radii[i], i);

	//cell centres connection "line" (green)
	du.DrawLine(gdID++, futureGeometry.centres[0],futureGeometry.centres[1], 2);
	for (int i=3; i < futureGeometry.noOfSpheres; ++i)
		du.DrawLine(gdID++, futureGeometry.centres[i-1],futureGeometry.centres[i], 2);
	if (futureGeometry.noOfSpheres > 4)
		du.DrawLine(gdID++, futureGeometry.centres[futureGeometry.noOfSpheres-1],futureGeometry.centres[2], 2);

	//off lines (red)
	for (int i=2; i < futureGeometry.noOfSpheres; ++i)
		du.DrawLine(ldID++, futureGeometry.centres[i],futureGeometry.centres[i]+offs[i-2], 1);

#ifdef DEBUG
	//draw s2s forces
	for (const auto& f : forcesForDisplay)
	{
		if (f.type == ftype_s2s)
			du.DrawVector(ldID++, f.base,f, (int)f.hint);
	}
#endif
}


void Nucleus2pNSAgent::drawForDebug(DisplayUnit& du)
{
#ifdef DEBUG
	int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID*40 +10000;

	//forces:
	for (const auto& f : forcesForDisplay)
	{
		int color = 2; //default color: green (for shape hinter)
		if      (f.type == ftype_body)      color = 4; //cyan
		else if (f.type == ftype_repulsive || f.type == ftype_drive) color = 5; //magenta
		else if (f.type == ftype_slide)     color = 6; //yellow
		else if (f.type == ftype_friction)  color = 3; //blue
		else if (f.type == ftype_s2s) color = -1; //don't draw
		if (color > 0) du.DrawVector(gdID++, f.base,f, color);
	}
#endif
}
