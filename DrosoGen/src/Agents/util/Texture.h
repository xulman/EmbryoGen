#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <cmath>
#include <i3d/image3d.h>
#include "../../util/report.h"
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

	/** Perlin in a box */
	/** crop to the geometry */


	// --------------------------------------------------
	// statistics about the texture


	// --------------------------------------------------
	// rendering

	/** renders the current content of the this->dots list into the given phantom image */
	void RenderIntoPhantom(i3d::Image3d<float> &phantoms);
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
