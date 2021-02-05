#include <iostream>
#include <cmath>
#include "../DisplayUnits/SceneryDisplayUnit.h"
#include "../Geometries/util/SpheresFunctions.h"


void reportSpheres(const Spheres& geom)
{
	std::cout << "-------- current config --------\n";
	for (auto i = 0; i < geom.getNoOfSpheres(); ++i)
		std::cout << i << ": @ " << geom.getCentres()[i] << ", R=" << geom.getRadii()[i] << "\n";
}


template <typename MT>
void renderIntoMask(i3d::Image3d<MT>& mask, const MT drawID,
                    const Spheres& geom)
{
	//shortcuts to the mask image parameters
	const Vector3d<float> res(mask.GetResolution().GetRes());
	const Vector3d<float> off(mask.GetOffset());

	const Vector3d<float>* centres = geom.getCentres();
	const float* radii = geom.getRadii();
	const int noOfSpheres = geom.getNoOfSpheres();

	//project and "clip" this AABB into the img frame
	//so that voxels to sweep can be narrowed down...
	//
	//   sweeping position and boundaries (relevant to the 'mask')
	Vector3d<size_t> curPos, minSweepPX,maxSweepPX;
	geom.AABB.exportInPixelCoords(mask, minSweepPX,maxSweepPX);
	//
	//micron coordinate of the running voxel 'curPos'
	Vector3d<float> centre;

	//sweep and check intersection with spheres' volumes
	for (curPos.z = minSweepPX.z; curPos.z < maxSweepPX.z; curPos.z++)
	for (curPos.y = minSweepPX.y; curPos.y < maxSweepPX.y; curPos.y++)
	for (curPos.x = minSweepPX.x; curPos.x < maxSweepPX.x; curPos.x++)
	{
		//get micron coordinate of the current voxel's centre
		centre.toMicronsFrom(curPos, res,off);

		//check the current voxel against all spheres
		for (int i = 0; i < noOfSpheres; ++i)
		{
			if ((centre-centres[i]).len() <= radii[i])
			{
				if (mask.GetVoxel(curPos.x,curPos.y,curPos.z) == 0)
					mask.SetVoxel(curPos.x,curPos.y,curPos.z, (MT)(drawID+i));
				break; //no need to test against the remaining spheres
			}
		}
	}
}

int main(void)
{
	//reference two balls
	Spheres twoS(2);

	//initial geom of the two balls
	twoS.updateCentre(0, Vector3d<float>(12.f,15.f,15.f));
	twoS.updateCentre(1, Vector3d<float>(26.f,15.f,15.f));
	twoS.updateRadius(0, 3.0);
	twoS.updateRadius(1, 4.5);

	//populate via LinkedSpheres
	SpheresFunctions::LinkedSpheres<float> builder(twoS, Vector3d<float>(0,1,0));

	//how many lines
	int numLines = 6;
	float stepAngle = 2.f * (float)M_PI / (float)numLines;
	float minAngle  = stepAngle / 2.f;
	float maxAngle  = 2.f * (float)M_PI;

	builder.defaultNoOfSpheresOnConnectionLines=2;
	//builder.resetAllAzimuthsToExtrusions(minAngle, stepAngle, maxAngle);
	builder.resetAllAzimuthsToExtrusions(minAngle, stepAngle, maxAngle, [](float){return 3;});

	builder.printPlan();
	REPORT("necessary cnt: " << builder.getNoOfNecessarySpheres());

	//finally, commit the case and draw it
	Spheres manyS(builder.getNoOfNecessarySpheres());
	builder.buildInto(manyS);
	manyS.updateOwnAABB();

	builder.printPlan();

	// ------------------------------------------------------
	reportSpheres(manyS);
	Vector3d<float> res(2);
	Vector3d<float> diag(40,30,30);
	i3d::Image3d<i3d::GRAY16> mask;
	//mask.SetOffset(i3d::Offset( diag.toI3dVector3d() ));
	mask.SetResolution( i3d::Resolution(res.toI3dVector3d()) );
	mask.MakeRoom( diag.elemMult(res).elemCeil().to<size_t>().toI3dVector3d() );
	const i3d::GRAY16 label = 20;

	std::cout << "Mask image, offset @ " << mask.GetOffset() << " microns, px size " << mask.GetSize() << "\n";
	renderIntoMask(mask,label,manyS);
	mask.SaveImage("mask.tif");

	return 0;
}
