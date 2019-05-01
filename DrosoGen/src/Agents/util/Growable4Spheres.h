#ifndef AGENTS_GROWABLE4SPHERES_H
#define AGENTS_GROWABLE4SPHERES_H

#include "../../Geometries/Spheres.h"

class Growable4Spheres: public Spheres
{
public:
	Growable4Spheres(void): Spheres(4) {};

	/** grow 4 sphere geometry in radius by 'dR', and in diameter by 'dD' */
	void growBy(const FLOAT dR, const FLOAT dD)
	{
		//make the nuclei fatter by 'dD' um in diameter
		for (int i=0; i < 4; ++i) radii[i] += dR;

		//offset the centres as well
		Vector3d<FLOAT> dispL1,dispL2;

		dispL1  = centres[2];
		dispL1 -= centres[1];
		dispL2  = centres[3];
		dispL2 -= centres[2];
		dispL1 *= dR / dispL1.len();
		dispL2 *= dD / dispL2.len();

		centres[2] += dispL1;
		centres[3] += dispL1;
		centres[3] += dispL2;

		dispL1 *= -1.0f;
		dispL2  = centres[0];
		dispL2 -= centres[1];
		dispL2 *= dD / dispL2.len();

		centres[1] += dispL1;
		centres[0] += dispL1;
		centres[0] += dispL2;
	}
};
#endif
