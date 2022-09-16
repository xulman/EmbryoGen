#pragma once

#include <memory>
#include <vector>
#include <cmath>
#include <i3d/image3d.h>
#include "../../util/report.hpp"
#include "../../util/rnd_generators.hpp"
#include "../../util/Dots.hpp"
#include "../../Geometries/Geometry.hpp"
#include "../../Geometries/Spheres.hpp"
#include "../../Geometries/util/SpheresFunctions.hpp"

/**
 * Utility class that maintains texture particles, e.g. fluorescence dots.
 */
class Texture
{
public:
	/** the main container of the dots for this texture */
	std::vector<Dot> dots;

	/** state of the random number generator that is associated with this texture instance */
	rndGeneratorHandle rngState;

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
report::debugMessage(fmt::format("increased capacity to {} dots" , dots.capacity()));
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
	void setupImageForRasterizingTexture( i3d::Image3d<VT>& img,
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
	void sampleDotsFromImage(const i3d::Image3d<VT>& img,
	                         const Spheres& geom,
	                         const VT quantization = 1);

	/** Populates the given 'geom' union of spheres with a Perlin texture. The texture
	    will be sampled/rendered at the given 'textureResolution' (in pixels/micron),
	    will be intensity-shifted to obtain the given 'textureAverageIntensity', and
	    will be created according to the four Perlin texture parameters ('var', 'alpha',
	    'beta', 'n'). The rendered texture will be sampled (converted into this->dots)
	    using the given 'quantization' factor. That means, the number of dots that occupy any
	    texture voxel is given by the voxel's intensity over the 'quantization': the higher
	    the parameter is, the fewer dots are created. If 'shouldCollectOutlyingDots' is true,
	    the created dots (incl. any possibly already present dots) are checked against
	    the given 'geom' and shifted if necessary to assure that they are always within
	    the given union of spheres (the 'geom').

	    Note that the Perlin noise created with the default parameters typically creates
	    images with intensities within the range [-0.5;+0.5]. The 'textureAverageIntensity'
	    is only shifting the range into [textureAverageIntensity-0.5,textureAverageIntensity+0.5].
	    Hence, with the higher value of the 'textureAverageIntensity', the contrast (ratio of
	    the highest over smallest intensity value) of the texture is essentially worsened. */
	void createPerlinTexture(const Spheres& geom,
	                         const Vector3d<G_FLOAT>& textureResolution,
	                         const double var,
	                         const double alpha = 8,
	                         const double beta = 4,
	                         const int n = 6,
	                         const float textureAverageIntensity = 1.0f,
	                         const float quantization = 0.1f,
	                         const bool shouldCollectOutlyingDots = true);

	// --------------------------------------------------
	// manipulating the texture

	/** find dots that are outside the given geometry and "put them back",
	    which is randomly close to the centre of the closest sphere,
	    returns the number of such processed dots (for statistics purposes) */
	int collectOutlyingDots(const Spheres& geom);

	// --------------------------------------------------
	// statistics about the texture

	// --------------------------------------------------
	// rendering

	/** renders the current content of the this->dots list into the given phantom image */
	void renderIntoPhantom(i3d::Image3d<float> &phantoms, const float quantization = 1);
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
	                 const short quantumGainPerVoxel):
		Texture(expectedNoOfQuantumDots),
		boxSize(1.0f/imgRes.x, 1.0f/imgRes.y, 1.0f/imgRes.z),
		qCounts( getCountsPerAxis(imgRes,quantumGainPerVoxel) )
	{
report::debugMessage(fmt::format("Converting inputs to: boxSize={} um, qCounts={} sub-dots" , toString(boxSize), toString(qCounts)));
		checkCountsPerAxis();
	}

	Vector3d<short> getCountsPerAxis(const Vector3d<float>& imgRes, const short quantumGainPerVoxel) const
	{
		//dots per voxel
		float quantum = quantumGainPerVoxel;

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
throw report::rtError("Quantum cube must have at least one dot along every axis");
	}

	/** defines (axis-aligned) box to represent a volume of the one quantum box */
	const Vector3d<float> boxSize;

	/** counts of quantum dots along every axis, multiplying the .x, .y and .z gives
	    the total contribution of this one quantum box */
	const Vector3d<short> qCounts;

	/** renders (quantum-wise) the current content of the this->dots list into the given phantom image,
	    the contributed intensity to the image should be 'quantum'-times greater than what would provide
		 the upstream, non-quantum Texture::renderIntoPhantom(phantoms,1.0) */
	void renderIntoPhantom(i3d::Image3d<float> &phantoms);
};


/**
 * This class can update coordinates of the texture dots on-the-go as the underlying "4S"
 * geometry is developing. This class is dedicated specifically to the Nucleus4SAgent --
 * it is based on the same geometry assumptions. It is an utility class wrapped around
 * four pieces of the SpheresFunctions::CoordsUpdater that implements weighted displacement
 * of any texture dots based on their intersection(s) with the "4S" spheres (and the depths
 * of the dots inside these spheres).
 */
class TextureUpdater4S
{
public:
	/** init the class with the current 4S geometry */
	TextureUpdater4S(const Spheres& geom)
		: objectIsReady( testNoOfSpheres(geom) ),
		  cu{ SpheresFunctions::CoordsUpdater<G_FLOAT>(geom.centres[0],geom.radii[0], geom.centres[0] - geom.centres[1]),
		      SpheresFunctions::CoordsUpdater<G_FLOAT>(geom.centres[1],geom.radii[1], geom.centres[0] - geom.centres[2]),
		      SpheresFunctions::CoordsUpdater<G_FLOAT>(geom.centres[2],geom.radii[2], geom.centres[1] - geom.centres[3]),
		      SpheresFunctions::CoordsUpdater<G_FLOAT>(geom.centres[3],geom.radii[3], geom.centres[2] - geom.centres[3]) }
	{}

	/** this method tracks the 4S geometry changes and updates, in accord, the coordinates
	    of the given list of texture dots */
	void updateTextureCoords(std::vector<Dot>& dots, const Spheres& newGeom);

private:
	/** a flag whose existence allows us to trigger the testNoOfSpheres() method during
	    this class construction; officially, however, this is a flag to signal if the
	    class is initialized */
	const bool objectIsReady;

	/** this method can be regarded as a container of a code that is executed as the first
	    when an object of this class is constructed, the purpose here is to test if the input
	    geometry is valid for this class */
	bool testNoOfSpheres(const Spheres& geom)
	{
		if (geom.noOfSpheres != 4)
throw report::rtError("Cannot init updating of coordinates for non-four sphere geometry.");
		return true;
	}

	/** coordinate updaters, one per sphere */
	SpheresFunctions::CoordsUpdater<G_FLOAT> cu[4];

	/** aux arrays for the updateTextureCoords() */
	Vector3d<G_FLOAT> prevCentre[4];
	G_FLOAT           prevRadius[4];
};


/**
 * This class can update coordinates of the texture dots on-the-go as the underlying
 * "2+NS" geometry is developing. This class is designed for agents that utilize
 * an explicit body (polarity) axis and assume given two spheres are attached to/are
 * defining this axis. Technically, it is an utility class wrapped around 2+N pieces
 * of the SpheresFunctions::CoordsUpdater.
 *
 * The deal in "2+NS" geometry is that there is 2+N spheres and that some two spheres
 * (through their centres) define the current orientation of the agent, which is then
 * imposed on all spheres.
 */
class TextureUpdater2pNS
{
public:
	/** init the class with the current 2pNS geometry */
	TextureUpdater2pNS(const Spheres& geom, const int _sphereAtCentre=0, const int _sphereOnMainAxis=1)
		: sphereAtCentre(   testAndReturnSphereIdx(geom,_sphereAtCentre) ),
		  sphereOnMainAxis( testAndReturnSphereIdx(geom,_sphereOnMainAxis) ),
		  noOfSpheres(geom.noOfSpheres),
		  prevCentre(noOfSpheres), prevRadius(noOfSpheres),
		  __weights(new float[noOfSpheres])
	{
		//allocate and init properly
		const Vector3d<G_FLOAT> orientVec(geom.centres[sphereOnMainAxis]-geom.centres[sphereAtCentre]);
		cu.reserve(noOfSpheres);
		for (int i=0; i < noOfSpheres; ++i)
			cu.emplace_back(geom.centres[i],geom.radii[i],orientVec);
	}

	/** source of a vector -- this together defines the body main axis, the cell's polarity if you will */
	const int sphereAtCentre;
	/** end of a vector -- this together defines the body main axis, the cell's polarity if you will */
	const int sphereOnMainAxis;

	/** total number of spheres, including the special two above; this would have been
	    a template parameter if I were able to make this (otherwise templated) class
	    a friend of the Spheres class .. I had issues with the template params */
	const int noOfSpheres;

	/** this method tracks the 2pNS geometry changes and updates, in accord, the coordinates
	    of the given list of texture dots */
	void updateTextureCoords(std::vector<Dot>& dots, const Spheres& newGeom);

private:
	int testAndReturnSphereIdx(const Spheres& geom, const int idx)
	{
		if (geom.noOfSpheres < 2)
throw report::rtError("Cannot init updating of coordinates, needs geometry of two or more spheres.");
		if (idx < 0 || idx >= geom.noOfSpheres)
throw report::rtError(fmt::format("Invalid sphere idx ({}) when {} spheres avail.", idx, geom.noOfSpheres));
		return idx;
	}

	/** coordinate updaters, one per sphere */
	std::vector< SpheresFunctions::CoordsUpdater<G_FLOAT> > cu;

	/** aux arrays for the updateTextureCoords() */
	std::vector< Vector3d<G_FLOAT> > prevCentre;
	std::vector< G_FLOAT >           prevRadius;

	/** aux array to be allocated once and not repeatedly in updateTextureCoords();
	    despite private, gcc finds it in derived classes and complains about confusion
	    with NucleusAgent::weights which is why the attribute's name got crippled... */
	std::unique_ptr<float[]> __weights;
};


class TextureUpdaterNS
{
public:
	/** init the class with the current NS geometry */
	TextureUpdaterNS(const Spheres& geom, int maxNoOfNeighs = 1)
		: noOfSpheres( testAndGetNoOfSpheres(geom) ),
		  neigWeightMatrix(noOfSpheres),
		  prevCentre(noOfSpheres), prevRadius(noOfSpheres),
		  prevOrientation(noOfSpheres),
		  __weights(new float[noOfSpheres])
	{
		reset(geom, maxNoOfNeighs);
	}

	/** total number of spheres, including the special two; this would have been
	    a template parameter if I were able to make this (otherwise templated) class
	    a friend of the Spheres class .. I had issues with the template params */
	const int noOfSpheres;

	/** extract "orientation" (into 'orientVec') of 'idx'-sphere in the given 'spheres' geometry */
	void getLocalOrientation(const Spheres& spheres, const int idx, Vector3d<G_FLOAT>& orientVec);

	/** this method tracks the NS geometry changes and updates, in accord, the coordinates
	    of the given list of texture dots */
	void updateTextureCoords(std::vector<Dot>& dots, const Spheres& newGeom);

	/** allocate and init properly */
	void reset(const Spheres& geom, int maxNoOfNeighs = 1)
	{
		//NB: this method also tests for sanity, so we don't have to do it here
		resetNeigWeightMatrix(geom,maxNoOfNeighs);

		Vector3d<G_FLOAT> orientVec;
		cu.clear();
		cu.reserve(noOfSpheres);
		for (int i=0; i < noOfSpheres; ++i)
		{
			getLocalOrientation(geom,i,orientVec);
			cu.emplace_back(geom.centres[i],geom.radii[i],orientVec);
		}

	}

	/** For each sphere, all overlapping (that is, neighboring) spheres are sorted
	    first according to their radii, and according to the size of the overlap of
	    two spheres if both are of the same radius; larger means earlier in the sorted
	    list. Finally, 'maxNoOfNeighs' of spheres (or less if that many is not
	    available) are marked in the 'neigWeightMatrix', all with weight 1. */
	void resetNeigWeightMatrix(const Spheres& spheres, int maxNoOfNeighs = 1);

	/** an alternative to resetNeigWeightMatrix() */
	void setNeigWeightMatrix(const SpheresFunctions::SquareMatrix<G_FLOAT>& newWeightMatrix);

	/** prints on the console */
	void printNeigWeightMatrix();

private:
	/** this method can be regarded as a container of a code that is executed as the first
	    when an object of this class is constructed, the purpose here is to test if the input
	    geometry is valid for this class, and to fill (const'ed) this.noOfSpheres */
	int testAndGetNoOfSpheres(const Spheres& geom)
	{
		if (geom.noOfSpheres < 2)
throw report::rtError("Cannot init updating of coordinates, needs geometry of two or more spheres.");
		return geom.noOfSpheres;
	}

	/** coordinate updaters, one per sphere */
	std::vector< SpheresFunctions::CoordsUpdater<G_FLOAT> > cu;

	/** neighbors and their weights, in this implementation the weights are binary:
	    a sphere is influenced by another one sphere (or equally by a few others) */
	SpheresFunctions::SquareMatrix<G_FLOAT> neigWeightMatrix;

	/** aux arrays for the updateTextureCoords() */
	std::vector< Vector3d<G_FLOAT> > prevCentre;
	std::vector< G_FLOAT >           prevRadius;
	std::vector< Vector3d<G_FLOAT> > prevOrientation;

	/** aux array to be allocated once and not repeatedly in updateTextureCoords();
	    despite private, gcc finds it in derived classes and complains about confusion
	    with NucleusAgent::weights which is why the attribute's name got crippled... */
	std::unique_ptr<float[]> __weights;
};
