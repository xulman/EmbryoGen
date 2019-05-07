#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include "../../util/rnd_generators.h"
#include "../../util/Dots.h"

/**
 * Utility class that maintains texture particles, e.g. fluorescence dots.
 */
class Texture
{
public:
	/** the main container of the dots for this texture */
	std::vector<Dot> dots;

	Texture(const unsigned int expectedNoOfDots)
	{
		dots.reserve(expectedNoOfDots);
	}

	/** checks the remaining free capacity of the dots container if it can
	    hold the expected no. of future new dots, and increase the capacity
	    if the outlook is not good -- it is better to increase the capacity
	    of the contained in larger chunks rather than increasing it in a
	    dot by dot manner (= many small increases); returns the new capacity */
	size_t checkAndIncreaseCapacity(const unsigned int expectedNoOfNewDots = 10000)
	{
		if (dots.capacity() - dots.size() < expectedNoOfNewDots)
			dots.reserve(dots.capacity() + expectedNoOfNewDots);

		return dots.capacity();
	}

	/** returns the number of dots currently stored in this Texture */
	size_t size(void) const
	{
		return dots.size();
	}

	size_t capacity(void) const
	{
		return dots.capacity();
	}

	// --------------------------------------------------
	// initializing the texture

	/** Perlin in a box */
	/** crop to the geometry */


	// --------------------------------------------------
	// statistics about the texture


	// --------------------------------------------------
	// rendering

//do not uncomment yet
//#define GTGEN_WITH_PHOTOBLEACHING
#ifdef GTGEN_WITH_PHOTOBLEACHING
	//fake function for now
	float GetBleachFactor(const short, const short)
	{
		return 0.5f;
	}
#endif

	template <class PV>
#ifdef GTGEN_WITH_PHOTOBLEACHING
	void RenderIntoPhantom(i3d::Image3d<PV> &phantoms, const int frameCnt)
	{
		DEBUG_REPORT("going to render " << dots.size() << " dots with photobleaching ON");
#else
	void RenderIntoPhantom(i3d::Image3d<PV> &phantoms, const int)
	{
		DEBUG_REPORT("going to render " << dots.size() << " dots with photobleaching OFF");
#endif

		//cache some output image params
		const Vector3d<float>  res(phantoms.GetResolution().GetRes());
		const Vector3d<float>  off(phantoms.GetOffset());
		const Vector3d<size_t> imgSize(phantoms.GetSize());
		Vector3d<size_t> imgPos;

#ifdef DEBUG
		auto stopWatch = tic();
#endif

#ifdef GTGEN_WITH_PHOTOBLEACHING
		// For the purpose of photobleaching we need to compute the exponential decay
		// for each individual fluorophore molecule. Therefore, we need a image buffer
		// with floating point datatype. After collecting all the pieces of information
		// together, we will convert this floating point data into grayscale
		i3d::Image3d<float> floatVoxels;
		floatVoxels.CopyMetaData(phantoms);
		floatVoxels.GetVoxelData() = 0.0f;
		float* const faddr = floatVoxels.GetFirstVoxelAddr();

		// a variable for debugging the photobleaching
		float meanIntensityCounter = 0.0f;

		for (const auto& dot : dots)
		{
			// turn micron position to phantom pixel one
			dot.pos.toPixels(imgPos, res,off);

			// is the pixel inside the input image?
			if (floatVoxels.Include((int)imgPos.x,(int)imgPos.y,(int)imgPos.z))
			{
				// find the bleached value of the current molecule
				const float fvalue = GetBleachFactor(dot.birth, (short)frameCnt);
				meanIntensityCounter += fvalue;

				// add the results together
				*(faddr + imgPos.toImgIndex(imgSize)) += fvalue;
			}
			else
			{
				DEBUG_REPORT("ups, dot " << dot.pos << " is outside the scene");
			}
		}

		DEBUG_REPORT(" total cell intensity: " << meanIntensityCounter <<
		             ", mean dot intensity: " << meanIntensityCounter/dots.size());
		             //well, the mean is okay only if there were no 'ups' above

		// copy the currently rendered phantom into image "phantoms", but
		//insert only non-zero (only contributing!) values (also not to overwrite
		//neighbouring agents)
		PV* const paddr = phantoms.GetFirstVoxelAddr();
		for (size_t i=0; i<phantoms.GetImageSize(); i++)
		{
			if (*(faddr+i) > 0) //only non-zero values
				*(paddr+i) = (PV)std::round(*(faddr+i));
		}
#else
		PV* const paddr = phantoms.GetFirstVoxelAddr();

		for (const auto& dot : dots)
		{
			// turn micron position to phantom pixel one
			dot.pos.toPixels(imgPos, res,off);

			// is the pixel inside the input image?
			if (phantoms.Include((int)imgPos.x,(int)imgPos.y,(int)imgPos.z))
			{
				// add the results together
				*(paddr + imgPos.toImgIndex(imgSize)) += 1;
			}
			else
			{
				DEBUG_REPORT("ups, dot " << dot.pos << " is outside the scene");
			}
		}
#endif

#ifdef DEBUG
		REPORT("rendering dots took " << toc(stopWatch));
#endif
	}
};
#endif
