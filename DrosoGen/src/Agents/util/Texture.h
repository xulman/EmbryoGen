#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <i3d/image3d.h>
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
#endif
