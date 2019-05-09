#include <cmath>
#include "Texture.h"
#include "../../util/report.h"

/** enable/disable the photobleaching feature */
//#define ENABLED_PHOTOBLEACHING

/** implements some fake function 1 -> 0+ to (poorly) simulate the ever
    decreasing photon budget, or just always returns 1.0 iff photobleaching
    shall be ignored (photobleaching feature is disabled */
#ifdef ENABLED_PHOTOBLEACHING
inline float GetBleachFactor(const short cntOfExcitations)
{
	return std::exp((float)-cntOfExcitations);
}
#else
inline float GetBleachFactor(const short)
{
	return 1.0f;
}
#endif


void Texture::RenderIntoPhantom(i3d::Image3d<float> &phantoms)
{
	DEBUG_REPORT("going to render " << dots.size() << " dots");

	//cache some output image params
	const Vector3d<float>  res(phantoms.GetResolution().GetRes());
	const Vector3d<float>  off(phantoms.GetOffset());
	const Vector3d<size_t> imgSize(phantoms.GetSize());
	Vector3d<size_t> imgPos;

#ifdef DEBUG
	auto stopWatch = tic();

	// a variable for debugging the photobleaching
	double meanIntContribution = 0.0f;
	long meanIntContCounter = 0;
#endif

	float* const paddr = phantoms.GetFirstVoxelAddr();
	for (auto& dot : dots)
	{
		//note that this dot is yet again excited to give some light
		++(dot.cntOfExcitations);

		// turn micron position to phantom pixel one
		dot.pos.toPixels(imgPos, res,off);

		// is the pixel inside the input image?
		//stdandard way:
		//if (phantoms.Include((int)imgPos.x,(int)imgPos.y,(int)imgPos.z))
		//
		//or, lower bound (but for _unsigned_ makes no sense):
		//if (imgPos.elemIsPredicateTrue([](const size_t x){ return x >= 0; })
		//
		//plus upper bound (tests also underflows... "negative" coordinates)
		if (imgPos.elemIsLessThan(imgSize))
		{
			const float fval = GetBleachFactor(dot.cntOfExcitations);
#ifdef DEBUG
			meanIntContribution += fval;
			++meanIntContCounter;
#endif
			// add the results together
			*(paddr + imgPos.toImgIndex(imgSize)) += fval;
		}
		else
		{
			DEBUG_REPORT("ups, dot " << dot.pos << " is outside the phantom image");
		}
	}

#ifdef DEBUG
	REPORT("added total cell intensity: " << meanIntContribution <<
	       " with mean dot intensity: " << meanIntContribution/(double)meanIntContCounter);
	REPORT("rendering dots took " << toc(stopWatch));
#endif
}


void TextureQuantized::RenderIntoPhantom(i3d::Image3d<float> &phantoms)
{
	DEBUG_REPORT("going to render " << dots.size() << " quantum dots");

	//cache some output image params
	const Vector3d<float>  res(phantoms.GetResolution().GetRes());     //in px/um
	const Vector3d<float>  off(phantoms.GetOffset());                  //in um

	//sweeping-related vars
	const int xLine   = (signed)phantoms.GetSizeX();                   //in px
	const int xyPlane = (signed)phantoms.GetSizeY() *xLine;            //in px
	const Vector3d<float> boxStep(
		Vector3d<float>( boxSize.x/(float)qCounts.x,
		                 boxSize.y/(float)qCounts.y,
		                 boxSize.z/(float)qCounts.z ).elemMult(res) );  //in px
	Vector3d<float> imgPos;                                            //in px

	//image boundaries after "applying retention zone"
	const Vector3d<float> halfBoxSize( (0.5f * boxSize).elemMult(res) ); //also a lower bound, px
	const Vector3d<float> imgSize(                                       //       upper bound, px
		Vector3d<size_t>(phantoms.GetSize()).to<float>().elemMult(res) - halfBoxSize );

#ifdef DEBUG
	auto stopWatch = tic();

	// a variable for debugging the photobleaching
	double meanIntContribution = 0.0f;
	const float quantumElems = qCounts.x * qCounts.y * qCounts.z;
	long meanIntContCounter = 0;
#endif

	float* const paddr = phantoms.GetFirstVoxelAddr();
	for (auto& dot : dots)
	{
		//note that this dot is yet again excited to give some light
		++(dot.cntOfExcitations);

		// turn micron position to phantom pixel one
		imgPos.from(dot.pos).toPixels(res,off);

		//test against image boundaries now and skip this dot if it does not
		//fall _completely_ inside the phantom image; otherwise we would need
		//to conduct such test with every single quantum -> very slow; this
		//does not hurt since the occurrence of the agent near (essentially
		//touching) the boundary should be rather unlikely
		if (imgPos.elemIsGreaterOrEqualThan(halfBoxSize)
		 && imgPos.elemIsLessThan(imgSize))
		{
			const float fval = GetBleachFactor(dot.cntOfExcitations);
#ifdef DEBUG
			meanIntContribution += quantumElems * fval;
			++meanIntContCounter;
#endif
			//std::cout << "--- centre " << imgPos << " um\n";

			//iterate the quantum box from this (float) voxel position
			imgPos -= halfBoxSize;

			// populate the "quantum cube":
			for (short iz=0; iz < qCounts.z; ++iz)
			{
				//NB: +0.5 is to achieve proper-rounding when casting to int does down-rounding
				const int Z = int(imgPos.z + (float)iz*boxStep.z +0.5f);

				for (short iy=0; iy < qCounts.y; ++iy)
				{
					const int Y = int(imgPos.y + (float)iy*boxStep.y +0.5f);

					float* const T = paddr + Z*xyPlane + Y*xLine;
					for (short ix=0; ix < qCounts.x; ++ix)
					{
						const int X = int(imgPos.x + (float)ix*boxStep.x +0.5f);
						T[X] += fval;;

						//std::cout << "populating at [" << X << "," << Y << "," << Z << "] px because "
						//          << ( imgPos+Vector3d<float>(ix,iy,iz).elemMult(boxStep) ) << " um\n";
					}
				}
			}
		}
		else
		{
			DEBUG_REPORT("ups, dot " << dot.pos << " is outside the phantom image");
		}
	}

#ifdef DEBUG
	REPORT("added total cell intensity: " << meanIntContribution <<
	       " with mean dot intensity: " << meanIntContribution/(double)meanIntContCounter);
	REPORT("rendering quantum dots took " << toc(stopWatch));
#endif
}
