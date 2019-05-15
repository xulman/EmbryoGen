#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <cmath>
#include <i3d/image3d.h>
#include "../../util/report.h"
#include "../../util/rnd_generators.h"
#include "../../util/Dots.h"
#include "../../Geometries/Geometry.h"
#include "../../Geometries/Spheres.h"

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
		{
			dots.reserve(dots.capacity() + expectedNoOfNewDots);
			DEBUG_REPORT("increased capacity to " << dots.capacity());
		}

		return dots.capacity();
	}

	/** shortcut method: returns the number of dots currently stored in this Texture */
	size_t size(void) const
	{
		return dots.size();
	}

	/** shortcut method: returns the total capacity of how many dots can be currently
	    stored in this Texture before container-capacity-increase-machinery gets triggered */
	size_t capacity(void) const
	{
		return dots.capacity();
	}

	// --------------------------------------------------
	// initializing the texture

	/** setup the output image to wrap around the given geometry, respecting
	    the given outer frame (in pixels) and the wanted resolution of the image */
	template <typename VT>
	void SetupImageForRasterizingTexture( i3d::Image3d<VT>& img,
	               const Vector3d<float>& imgRes,
	               const Geometry& geom,
	               const Vector3d<short>& pxFrameWidth = Vector3d<short>(2) )
	const
	{
		geom.AABB.adaptImage(img, imgRes,pxFrameWidth);
	}

	/** populate (actually add, not reset) the dot list by sampling the rasterized
	    texture (stored in 'img') respecting the actual shape of the agent ('geom'),
	    and considering given 'quantization' */
	template <typename VT>
	void SampleDotsFromImage(const i3d::Image3d<VT>& img,
	                         const Spheres& geom,
	                         const VT quantization = 1)
	{
		const Vector3d<float> res( img.GetResolution().GetRes() );
		const Vector3d<float> off( img.GetOffset() );

		//sigmas such that Gaussian will spread just across one voxel
		const Vector3d<float> chaossSigma( Vector3d<float>(1.0f/6.0f).elemDivBy(res) );
		rndGeneratorHandle chaossState;
#ifdef DEBUG
		long missedDots = 0;
#endif

		//sweeping stuff...
		const VT* imgPtr = img.GetFirstVoxelAddr();
		Vector3d<size_t> pxPos;
		Vector3d<float>  umPos;

		for (pxPos.z = 0; pxPos.z < img.GetSizeZ(); ++pxPos.z)
		for (pxPos.y = 0; pxPos.y < img.GetSizeY(); ++pxPos.y)
		for (pxPos.x = 0; pxPos.x < img.GetSizeX(); ++pxPos.x, ++imgPtr)
		{
			umPos.toMicronsFrom(pxPos, res,off);

			if (geom.collideWithPoint(umPos) >= 0)
			{
				checkAndIncreaseCapacity();

				//displace randomly within this voxel
				for (int i = int(*imgPtr/quantization); i > 0; --i)
				{
					dots.push_back(umPos);
					dots.back().pos += Vector3d<float>(
					       GetRandomGauss(0.f,chaossSigma.x, chaossState),
					       GetRandomGauss(0.f,chaossSigma.y, chaossState),
					       GetRandomGauss(0.f,chaossSigma.z, chaossState) );

#ifdef DEBUG
					//test if the new position still fits inside the original voxel
					Vector3d<size_t> pxBackPos;
					dots.back().pos.fromMicronsTo(pxBackPos, res,off);
					if (! pxBackPos.elemIsPredicateTrue(pxPos, [](float l,float r){ return l==r; }))
						++missedDots;
#endif
				}
			}
		}

#ifdef DEBUG
		REPORT("there are " << missedDots << " (" << 100.f*missedDots/dots.size()
		       << "%) dots placed outside its original voxel");
#endif
	}


	// --------------------------------------------------
	// statistics about the texture


	// --------------------------------------------------
	// rendering

	/** renders the current content of the this->dots list into the given phantom image */
	void RenderIntoPhantom(i3d::Image3d<float> &phantoms, const float quantization = 1);
};

/**
 * Utility class that maintains texture particles, e.g. fluorescence dots, that assumes
 * that one such particle represents a 'quantum' amount of (equally exposed/bleached)
 * dots. The quantum is defined as a number of fluorescence molecules equally spread
 * throughout given (axis aligned) box. Often, one chooses the box size to correspond
 * with the volume of one voxel in the phantom image. Furthermore, the number of the
 * fl. molecules (the quantum size) is a compromise to obtain phantom images with
 * desired intensity levels while maintaining computational tractability (that is,
 * not having exceedingly long list with (small) quanta).
 */
class TextureQuantized : public Texture
{
public:
	/** c'tor that requires explicitly the number of fl. molecules per volumetric box */
	TextureQuantized(const unsigned int expectedNoOfQuantumDots,
	                 const Vector3d<float>& _boxSize,
	                 const Vector3d<short>& _qCounts):
		Texture(expectedNoOfQuantumDots),
		boxSize(_boxSize), qCounts(_qCounts)
	{
		checkCountsPerAxis();
	}

	/** a utility c'tor that estimates the quantum box from voxel size and, essentially,
	    a phantom image intensity increase/gain */
	TextureQuantized(const unsigned int expectedNoOfQuantumDots,
	                 const Vector3d<float>& imgRes,
	                 const short quantumGain):
		Texture(expectedNoOfQuantumDots),
		boxSize(1.0f/imgRes.x, 1.0f/imgRes.y, 1.0f/imgRes.z),
		qCounts( getCountsPerAxis(imgRes,quantumGain) )
	{
		DEBUG_REPORT("Converting inputs to: boxSize=" << boxSize <<", qCounts=" << qCounts);
		checkCountsPerAxis();
	}

	Vector3d<short> getCountsPerAxis(const Vector3d<float>& imgRes, const short quantumGain) const
	{
		//dots per voxel
		float quantum = quantumGain;

		//density in dots/um^3
		quantum *= imgRes.x*imgRes.y*imgRes.z;

		//dots (density) per one axis
		quantum = std::cbrt(quantum);

		//how many dots per axis, honours anisotropy (if present)
		Vector3d<short> counts;
		counts.x = (short)std::ceil(quantum * boxSize.x);
		counts.y = (short)std::ceil(quantum * boxSize.y);
		counts.z = (short)std::ceil(quantum * boxSize.z);
		//or: counts.from( (quantum * boxSize).elemCeil() );
		//should hold x*y*z >= original input quantum, ideally closest possible to the equality

		return counts;
	}

	void checkCountsPerAxis() const
	{
		if (! qCounts.elemIsGreaterThan( Vector3d<short>(0) ) )
			throw ERROR_REPORT("Quantum cube must have at least one dot along every axis");
	}

	/** defines (axis-aligned) box to represent a volume of the one quantum box */
	const Vector3d<float> boxSize;

	/** counts of quantum dots along every axis, multiplying the .x, .y and .z gives
	    the total contribution of this one quantum box */
	const Vector3d<short> qCounts;

	/** renders (quantum-wise) the current content of the this->dots list into the given phantom image,
	    the contributed intensity to the image should be 'quantum'-times greater than what would provide
		 the upstream, non-quantum Texture::RenderIntoPhantom() */
	void RenderIntoPhantom(i3d::Image3d<float> &phantoms);
};
#endif
