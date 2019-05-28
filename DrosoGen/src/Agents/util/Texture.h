#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <cmath>
#include <i3d/image3d.h>
#include "../../util/report.h"
#include "../../util/rnd_generators.h"
#include "../../util/Dots.h"
#include "../../util/texture/texture.h"
#include "../../Geometries/Geometry.h"
#include "../../Geometries/Spheres.h"
#include "../../Geometries/util/SpheresFunctions.h"

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
			DEBUG_REPORT("increased capacity to " << dots.capacity() << " dots");
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
					       GetRandomGauss(0.f,chaossSigma.x, rngState),
					       GetRandomGauss(0.f,chaossSigma.y, rngState),
					       GetRandomGauss(0.f,chaossSigma.z, rngState) );

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
		       << " %) dots placed outside its original voxel");
		REPORT("there are currently " << dots.size() << " registered dots");
#endif
	}

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
	void CreatePerlinTexture(const Spheres& geom,
	                         const Vector3d<FLOAT> textureResolution,
	                         const double var,
	                         const double alpha = 8,
	                         const double beta = 4,
	                         const int n = 6,
	                         const float textureAverageIntensity = 1.0f,
	                         const float quantization = 0.1f,
	                         const bool shouldCollectOutlyingDots = true)
	{
		//setup the aux texture image
		i3d::Image3d<float> img;
		SetupImageForRasterizingTexture(img,textureResolution, geom);

		//sanity check... if the 'geom' is "empty", no texture image is "wrapped" around it,
		//we do no creation of the texture then...
		if (img.GetImageSize() == 0)
		{
			DEBUG_REPORT("WARNING: Wrapping texture image is of zero size... stopping here.");
			return;
		}

		//fill the aux image
		DoPerlin3D(img, var,alpha,beta,n);

		//get the current average intensity and adjust to the desired one
		double sum = 0;
		float* i = img.GetFirstVoxelAddr();
		float* const iL = i + img.GetImageSize();
		for (; i != iL; ++i) sum += *i;
		sum /= (double)img.GetImageSize();
		//
		const float textureIntShift = textureAverageIntensity - (float)sum;
		for (i = img.GetFirstVoxelAddr(); i != iL; ++i)
		{
			*i += textureIntShift;
			//*i  = std::max( *i, 0.f );
		}
		SampleDotsFromImage(img,geom, quantization);
		//NB: the function ignores any negative-valued texture image voxels,
		//    no dots are created for such voxels

		if (shouldCollectOutlyingDots)
		{
			const int dotOutliers = CollectOutlyingDots(geom);
			DEBUG_REPORT(dotOutliers << " (" << 100.f*dotOutliers/dots.size()
			             << " %) dots had to be moved inside the initial geometry");
		}
	}

	// --------------------------------------------------
	// manipulating the texture

	/** find dots that are outside the given geometry and "put them back",
	    which is randomly close to the centre of the closest sphere,
	    returns the number of such processed dots (for statistics purposes) */
	int CollectOutlyingDots(const Spheres& geom)
	{
		int count = 0;
#ifdef DEBUG
		double outDist = 0;
		double inDist  = 0;
		int postCorrectionsCnt = 0;

		auto stopWatch = tic();
#endif

		Vector3d<FLOAT> tmp;
		FLOAT tmpLen;

		for (auto& dot : dots)
		{
			bool foundInside = false;
			FLOAT nearestDist = TOOFAR;
			int nearestIdx  = -1;

			for (int i=0; i < geom.noOfSpheres && !foundInside; ++i)
			{
				//test against the i-th sphere
				tmp  = geom.centres[i];
				tmp -= dot.pos;
				tmpLen = tmp.len() - geom.radii[i];

				foundInside = tmpLen <= 0;

				if (!foundInside && tmpLen < nearestDist)
				{
					//update nearest distance
					nearestDist = tmpLen;
					nearestIdx  = i;
				}
			}

			if (!foundInside)
			{
				//correct dot's position according to the nearestIdx:
				//random position inside the (zero-centered) sphere
				dot.pos.x = GetRandomGauss(0.f,geom.radii[nearestIdx]/2.f, rngState);
				dot.pos.y = GetRandomGauss(0.f,geom.radii[nearestIdx]/2.f, rngState);
				dot.pos.z = GetRandomGauss(0.f,geom.radii[nearestIdx]/2.f, rngState);

				//make sure we're inside this sphere
				if (dot.pos.len() > geom.radii[nearestIdx])
				{
					dot.pos.changeToUnitOrZero() *= 0.9f * geom.radii[nearestIdx];
					//NB: it held and holds for sure: dot.pos.len() > 0
#ifdef DEBUG
					++postCorrectionsCnt;
#endif
				}

#ifdef DEBUG
				outDist += nearestDist;
				inDist  += dot.pos.len();
#endif
				dot.pos += geom.centres[nearestIdx];
				++count;
			}
		}

#ifdef DEBUG
		if (count > 0)
		{
			REPORT("average outside-to-surface distance " << outDist/(double)count << " um (in " << toc(stopWatch) << ")");
			REPORT("average new-pos-to-centre  distance " <<  inDist/(double)count << " um");
			REPORT("secondary corrections of " << postCorrectionsCnt << "/" << count
			       << " (" << 100.f*postCorrectionsCnt/count << " %) dots");
		}
		else
		{
			REPORT("no corrections were necessary (in " << toc(stopWatch) << ")");
		}
#endif
		return count;
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
	                 const short quantumGainPerVoxel):
		Texture(expectedNoOfQuantumDots),
		boxSize(1.0f/imgRes.x, 1.0f/imgRes.y, 1.0f/imgRes.z),
		qCounts( getCountsPerAxis(imgRes,quantumGainPerVoxel) )
	{
		DEBUG_REPORT("Converting inputs to: boxSize=" << boxSize <<" um, qCounts=" << qCounts << " sub-dots");
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
			throw ERROR_REPORT("Quantum cube must have at least one dot along every axis");
	}

	/** defines (axis-aligned) box to represent a volume of the one quantum box */
	const Vector3d<float> boxSize;

	/** counts of quantum dots along every axis, multiplying the .x, .y and .z gives
	    the total contribution of this one quantum box */
	const Vector3d<short> qCounts;

	/** renders (quantum-wise) the current content of the this->dots list into the given phantom image,
	    the contributed intensity to the image should be 'quantum'-times greater than what would provide
		 the upstream, non-quantum Texture::RenderIntoPhantom(phantoms,1.0) */
	void RenderIntoPhantom(i3d::Image3d<float> &phantoms);
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
		  cu{ SpheresFunctions::CoordsUpdater<FLOAT>(geom.centres[0],geom.radii[0], geom.centres[0] - geom.centres[1]),
		      SpheresFunctions::CoordsUpdater<FLOAT>(geom.centres[1],geom.radii[1], geom.centres[0] - geom.centres[2]),
		      SpheresFunctions::CoordsUpdater<FLOAT>(geom.centres[2],geom.radii[2], geom.centres[1] - geom.centres[3]),
		      SpheresFunctions::CoordsUpdater<FLOAT>(geom.centres[3],geom.radii[3], geom.centres[2] - geom.centres[3]) }
	{}

	/** this method tracks the 4S geometry changes and updates, in accord, the coordinates
	    of the given list of texture dots */
	void updateTextureCoords(std::vector<Dot>& dots, const Spheres& newGeom)
	{
#ifdef DEBUG
		if (newGeom.noOfSpheres != 4)
			throw ERROR_REPORT("Cannot update coordinates for non-four sphere geometry.");
#endif
		// backup: last geometry for which user coordinates were valid
		for (unsigned int i=0; i < 4; ++i)
		{
			prevCentre[i] = cu[i].prevCentre;
			prevRadius[i] = cu[i].prevRadius;
		}

		//prepare the updating routines...
		cu[0].prepareUpdating( newGeom.centres[0], newGeom.radii[0],
		                       newGeom.centres[0] - newGeom.centres[1] );

		cu[1].prepareUpdating( newGeom.centres[1], newGeom.radii[1],
		                       newGeom.centres[0] - newGeom.centres[2] );

		cu[2].prepareUpdating( newGeom.centres[2], newGeom.radii[2],
		                       newGeom.centres[1] - newGeom.centres[3] );

		cu[3].prepareUpdating( newGeom.centres[3], newGeom.radii[3],
		                       newGeom.centres[2] - newGeom.centres[3] );

		//aux variables
		float weights[4];
		float sum;
		Vector3d<float> tmp,newPos;
#ifdef DEBUG
		int outsideDots = 0;
#endif
		//shift texture particles
		for (auto& dot : dots)
		{
			//determine the weights
			for (unsigned int i=0; i < 4; ++i)
			{
				tmp  = dot.pos;
				tmp -= prevCentre[i];
				weights[i] = std::max(prevRadius[i] - tmp.len(), (FLOAT)0);
			}

			//normalization factor
			sum = weights[0] + weights[1] + weights[2] + weights[3];

			if (sum > 0)
			{
				//apply the weights
				newPos = 0;
				for (unsigned int i=0; i < 4; ++i)
				if (weights[i] > 0)
				{
					tmp = dot.pos;
					cu[i].updateCoord(tmp);
					newPos += (weights[i]/sum) * tmp;
				}
				dot.pos = newPos;
			}
			else
			{
#ifdef DEBUG
				++outsideDots;
#endif
			}
		}

#ifdef DEBUG
		if (outsideDots > 0)
			REPORT(outsideDots << " could not be updated (no matching sphere found, weird...)");
#endif
	}

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
			throw ERROR_REPORT("Cannot init updating of coordinates for non-four sphere geometry.");
		return true;
	}

	/** coordinate updaters, one per sphere */
	SpheresFunctions::CoordsUpdater<FLOAT> cu[4];

	/** aux arrays for the updateTextureCoords() */
	Vector3d<FLOAT> prevCentre[4];
	FLOAT           prevRadius[4];
};
#endif
