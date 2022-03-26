#include <cmath>
#include "NucleusNSAgent.hpp"

void NucleusNSAgent::resetDistanceMatrix()
{
	for (int row=0; row < futureGeometry.noOfSpheres; ++row)
	{
		*distanceMatrix(row,row) = -1;
		for (int col=row+1; col < futureGeometry.noOfSpheres; ++col)
		{
			const float dist = (futureGeometry.centres[row] - futureGeometry.centres[col]).len();
			*distanceMatrix(row,col) = dist;
			*distanceMatrix(col,row) = dist;
		}
	}
}


void NucleusNSAgent::printDistanceMatrix()
{
	report::message(fmt::format("{} reference distances among spheres:", getSignature()));
	distanceMatrix.print();
}


void NucleusNSAgent::advanceAndBuildIntForces(const float futureGlobalTime)
{
	//tolerated mis-position (no "adjustment" s2s forces are created within this radius)
	const G_FLOAT keepCalmDistance = (G_FLOAT)0.1;
	//const G_FLOAT keepCalmDistanceSq = keepCalmDistance*keepCalmDistance;

#ifdef DEBUG
	if (futureGeometry.noOfSpheres != distanceMatrix.side)
throw report::rtError(fmt::format("distanceMatrix stores {} spheres but futureGeometry contains {}" , distanceMatrix.side, futureGeometry.noOfSpheres));

	forces_s2sInducers.clear();
#endif

	Vector3d<G_FLOAT> forceVec;
	for (int row=0; row < futureGeometry.noOfSpheres; ++row)
	{
		for (int col=row+1; col < futureGeometry.noOfSpheres; ++col)
		{
			forceVec  = futureGeometry.centres[col];
			forceVec -= futureGeometry.centres[row];
			G_FLOAT diffDist = forceVec.len() - distanceMatrix.get(row,col);
			if (diffDist > keepCalmDistance || diffDist < -keepCalmDistance)
			{
				forceVec.changeToUnitOrZero();
				forceVec *= diffDist * fstrength_body_scale;
				forces.emplace_back( forceVec, futureGeometry.centres[row],row, ftype_s2s );
#ifdef DEBUG
				forces_s2sInducers.push_back(col);
#endif
				forceVec *= -1;
				forces.emplace_back( forceVec, futureGeometry.centres[col],col, ftype_s2s );
#ifdef DEBUG
				forces_s2sInducers.push_back(row);
#endif
			}
		}
	}

	NucleusAgent::advanceAndBuildIntForces(futureGlobalTime);
}


void NucleusNSAgent::drawMask(DisplayUnit& du)
{
	int dID  = DisplayUnit::firstIdForAgentObjects(ID);
#ifdef DEBUG
	int ldID = DisplayUnit::firstIdForAgentDebugObjects(ID);
#endif
	int gdID = DisplayUnit::firstIdForSceneDebugObjects() + ID*40 +5000;

	//draw spheres, each at different color
	for (int i=0; i < futureGeometry.noOfSpheres; ++i)
		du.DrawPoint(dID++, futureGeometry.centres[i],futureGeometry.radii[i], i);

	//cell centres connection "line" (green)
	for (int i=1; i < futureGeometry.noOfSpheres; ++i)
		du.DrawLine(gdID++, futureGeometry.centres[i-1],futureGeometry.centres[i], 2);

#ifdef DEBUG
	//draw s2s forces
	std::vector<int>::iterator s2s_color = forces_s2sInducers.begin();
	for (const auto& f : forcesForDisplay)
	{
		if (f.type == ftype_s2s)
			du.DrawVector(ldID++, f.base,f, *s2s_color++); //inducer's color
	}
#endif
}


#ifdef DEBUG
void NucleusNSAgent::drawForDebug(DisplayUnit& du)
{
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
}
#else
void NucleusNSAgent::drawForDebug(DisplayUnit&) {}
#endif
