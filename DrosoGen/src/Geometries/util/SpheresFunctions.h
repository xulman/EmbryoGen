#ifndef GEOMETRY_SPHERESFUNCTIONS_H
#define GEOMETRY_SPHERESFUNCTIONS_H

#include "../Spheres.h"

class SpheresFunctions
{
public:
	/** grow 4 sphere geometry 's' in radius by 'dR', and in diameter by 'dD' */
	static
	void grow4SpheresBy(Spheres& s, const FLOAT dR, const FLOAT dD)
	{
		if (s.noOfSpheres != 4)
			throw new std::runtime_error("SpheresFunctions::grow4SpheresBy(): Cannot grow non-four sphere geometry.");

		//make the nuclei fatter by 'dD' um in diameter
		for (int i=0; i < 4; ++i) s.radii[i] += dR;

		//offset the centres as well
		Vector3d<FLOAT> dispL1,dispL2;

		dispL1  = s.centres[2];
		dispL1 -= s.centres[1];
		dispL2  = s.centres[3];
		dispL2 -= s.centres[2];
		dispL1 *= dR / dispL1.len();
		dispL2 *= dD / dispL2.len();

		s.centres[2] += dispL1;
		s.centres[3] += dispL1;
		s.centres[3] += dispL2;

		dispL1 *= -1.0f;
		dispL2  = s.centres[0];
		dispL2 -= s.centres[1];
		dispL2 *= dD / dispL2.len();

		s.centres[1] += dispL1;
		s.centres[0] += dispL1;
		s.centres[0] += dispL2;
	}
};
#endif
