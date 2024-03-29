#include "Nucleus4SAgent.h"

void Nucleus4SAgent::getCurrentOffVectorsForCentres(Vector3d<G_FLOAT> offs[4])
{
	//the centre point
	Vector3d<G_FLOAT> refCentre(futureGeometry.centres[1]);
	refCentre += futureGeometry.centres[2];
	refCentre *= 0.5;

	//the axis/orientation between 2nd and 3rd sphere
	Vector3d<G_FLOAT> refAxis(futureGeometry.centres[2]);
	refAxis -= futureGeometry.centres[1];

	//make it half-of-the-expected-distance long
	refAxis *= 0.5f*centreDistance[1] / refAxis.len();

	//calculate how much are the 2nd and 3rd spheres off their expected positions
	offs[1]  = refCentre;
	offs[1] -= refAxis; //the expected position
	offs[1] -= futureGeometry.centres[1]; //the difference vector

	offs[2]  = refCentre;
	offs[2] += refAxis;
	offs[2] -= futureGeometry.centres[2];

	//calculate how much is the 1st sphere off its expected position
	offs[0]  = refCentre;
	offs[0] -= (centreDistance[0]/(0.5f*centreDistance[1]) +1.0f) * refAxis;
	offs[0] -= futureGeometry.centres[0];

	//calculate how much is the 4th sphere off its expected position
	offs[3]  = refCentre;
	offs[3] += (centreDistance[2]/(0.5f*centreDistance[1]) +1.0f) * refAxis;
	offs[3] -= futureGeometry.centres[3];
}


void Nucleus4SAgent::advanceAndBuildIntForces(const float futureGlobalTime)
{
	//check bending of the spheres (how much their position deviates from a line,
	//includes also checking the distance), and add, if necessary, another
	//forces to the list
	Vector3d<G_FLOAT> sOff[4];
	getCurrentOffVectorsForCentres(sOff);

	//tolerated mis-position (no "adjustment" s2s forces are created within this radius)
	const G_FLOAT keepCalmDistanceSq = (G_FLOAT)0.01; // 0.01 = 0.1^2

	if (sOff[0].len2() > keepCalmDistanceSq)
	{
		//properly scaled force acting on the 1st sphere: body_scale * len()
		sOff[0] *= fstrength_body_scale;
		forces.emplace_back( sOff[0], futureGeometry.centres[0],0, ftype_s2s );

		sOff[0] *= -1.0;
		forces.emplace_back( sOff[0], futureGeometry.centres[1],1, ftype_s2s );
	}

	if (sOff[1].len2() > keepCalmDistanceSq)
	{
		//properly scaled force acting on the 2nd sphere: body_scale * len()
		sOff[1] *= fstrength_body_scale;
		forces.emplace_back( sOff[1], futureGeometry.centres[1],1, ftype_s2s );

		sOff[1] *= -0.5;
		forces.emplace_back( sOff[1], futureGeometry.centres[0],0, ftype_s2s );
		forces.emplace_back( sOff[1], futureGeometry.centres[2],2, ftype_s2s );
	}

	if (sOff[2].len2() > keepCalmDistanceSq)
	{
		//properly scaled force acting on the 2nd sphere: body_scale * len()
		sOff[2] *= fstrength_body_scale;
		forces.emplace_back( sOff[2], futureGeometry.centres[2],2, ftype_s2s );

		sOff[2] *= -0.5;
		forces.emplace_back( sOff[2], futureGeometry.centres[1],1, ftype_s2s );
		forces.emplace_back( sOff[2], futureGeometry.centres[3],3, ftype_s2s );
	}

	if (sOff[3].len2() > keepCalmDistanceSq)
	{
		//properly scaled force acting on the 1st sphere: body_scale * len()
		sOff[3] *= fstrength_body_scale;
		forces.emplace_back( sOff[3], futureGeometry.centres[3],3, ftype_s2s );

		sOff[3] *= -1.0;
		forces.emplace_back( sOff[3], futureGeometry.centres[2],2, ftype_s2s );
	}

	NucleusAgent::advanceAndBuildIntForces(futureGlobalTime);
}


void Nucleus4SAgent::drawForDebug(DisplayUnit& du)
{
	//first the general NucleusAgent agent debug stuff...
	NucleusAgent::drawForDebug(du);

	//second the 4S-special additions...
	//(but also render only if under inspection)
	if (detailedDrawingMode)
	{
		int dID = DisplayUnit::firstIdForAgentDebugObjects(ID);
		dID += 32767; //we will be using IDs from the upper end of the reserved interval

		//shape deviations:
		//blue lines to show deviations from the expected geometry
		Vector3d<G_FLOAT> sOff[4];
		getCurrentOffVectorsForCentres(sOff);
		du.DrawLine(dID++, futureGeometry.centres[0],futureGeometry.centres[0]+sOff[0], 3);
		du.DrawLine(dID++, futureGeometry.centres[1],futureGeometry.centres[1]+sOff[1], 3);
		du.DrawLine(dID++, futureGeometry.centres[2],futureGeometry.centres[2]+sOff[2], 3);
		du.DrawLine(dID++, futureGeometry.centres[3],futureGeometry.centres[3]+sOff[3], 3);
	}
}
