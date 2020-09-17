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

	NucleusAgent::advanceAndBuildIntForces(futureGlobalTime);
}
