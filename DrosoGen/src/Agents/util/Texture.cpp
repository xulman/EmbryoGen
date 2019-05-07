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
		if (phantoms.Include((int)imgPos.x,(int)imgPos.y,(int)imgPos.z))
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
