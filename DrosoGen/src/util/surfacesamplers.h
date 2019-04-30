#ifndef SURFACESAMPLERS_H
#define SURFACESAMPLERS_H

#include <math.h>
#include <i3d/image3d.h>
#include <functional>

/**
 * This iteratively gives vectors/coordinates that (somewhat) regularly
 * sample a surface of a sphere that is placed in the coordinate centre.
 *
 * Init the sampler with a call to any of the two reset*() methods and
 * then keep getting surface points via next() until it returns with false.
 */
template <typename T>
class SphereSampler
{
public:
	/** restarts the surface sampling of a sphere with the given radius,
	    the circumference of a full radius should be populated with
	    the given 'noOfSteps' */
	void resetByCount(const T radius, const int noOfSteps = 12)
	{
		this->radius   = radius;
		this->stepSize = 2 * PI * radius / static_cast<T>(noOfSteps);

		theCommonInit();
	}

	/** restarts the surface sampling of a sphere with the given radius,
	    the circumference of a full radius should be populated with
	    the given 'stepSize' (in microns) */
	void resetByStepSize(const T radius, const T stepSize = (T)0.9)
	{
		this->radius   = radius;
		this->stepSize = stepSize;

		theCommonInit();
	}

	/** gives another point on the sphere's surface, returns false if
	    all surface points were examined */
	bool next(Vector3d<T>& surfacePoint)
	{
		//end of the outer cycle?
		if (climberRad >= PI) return false;

		//init the inner cycle?
		if (rad == 0)
		{
			radiusYZ = radius * std::sin(climberRad);
			radStep = radiusYZ > 0 ? stepSize / radiusYZ : 7; //NB: 7 > 2*PI to spawn just one (pole) point
		}

		//the meat of the inner cycle
		surfacePoint.x = radius   * std::cos(climberRad);
		surfacePoint.y = radiusYZ * std::cos(rad);
		surfacePoint.z = radiusYZ * std::sin(rad);
		rad += radStep;

		//end of the inner cycle? reset it and increment the outer cycle
		if (rad >= nearlyTwoPI)
		{
			rad = 0;
			climberRad += climberRadStep;
		}

		return true;

		/* this is the closed-form of the algorithm:
		//let's step/sample a curve that is actually 1/2 of a circle
		const T climberRadStep = stepSize / radius;
		for (climberRad = 0; climberRad < PI; climberRad += climberRadStep)
		{
			//let's step/sample a full circle whose radius is given by climberRad
			const T radiusYZ = radius * std::sin(climberRad);
			const T radStep = stepSize / radiusYZ;
			for (T rad = 0; rad < 2*PI; rad += radStep)
			{
				surfacePoint.x = radius   * std::cos(climberRad);
				surfacePoint.y = radiusYZ * std::cos(rad);
				surfacePoint.z = radiusYZ * std::sin(rad);
			}
		}
		*/
	}

private:
	constexpr static const T   PI        = (T)  M_PI;
	constexpr static const T nearlyTwoPI = (T) (2*M_PI -0.01);

	T radius;
	T stepSize;

	T climberRad, climberRadStep;
	T radiusYZ;
	T rad, radStep;

	void theCommonInit(void)
	{
		//init state variable for the next() method
		climberRad = 0;
		climberRadStep = stepSize / radius;
		rad = 0;
	}
};


/**
 * This iteratively gives vectors/coordinates that (somewhat) regularly
 * sample a surface (represented with a given voxel value) stored/rasterized
 * in an image. It essentially (and iteratively) will give vectors/coordinates
 * of centre of the pixels that satisfy user-given condition.
 *
 * Init the sampler with a call to any of the reset*() methods and
 * then keep getting surface points via next() until it returns with false.
 *
 * Note: ST - sampling type, VT - voxel type
 */
template <typename ST, typename VT>
class ImageSampler
{
public:
	void resetByVoxelStep(const i3d::Image3d<VT>& img,
	                      const std::function<bool(const VT)>& pxPredicate,
	                      const Vector3d<size_t>& pixelStep)
	{
		theCommonInit(img); //sets up the 'res'

		this->pxPredicate = &pxPredicate;
		this->micronStep.from(pixelStep).elemDivBy(res);

		DEBUG_REPORT("imgSize=" << img.GetSize() << " px, micronStep=" << this->micronStep << " um");
	}

	void resetByMicronStep(const i3d::Image3d<VT>& img,
	                       const std::function<bool(const VT)>& pxPredicate,
	                       const Vector3d<float>& micronStep)
	{
		theCommonInit(img);

		this->pxPredicate = &pxPredicate;
		this->micronStep = micronStep;

		DEBUG_REPORT("imgSize=" << img.GetSize() << " px, micronStep=" << this->micronStep << " um");
	}

	/** gives another point on the sphere's surface, returns false if
	    all surface points were examined */
	bool next(Vector3d<ST>& surfacePoint)
	{
		do {
			while (p != pL && (*pxPredicate)(*p) == false) ++p;
			if (p == pL) return false;

			//define surfacePoint from the (p-pF) offset
			size_t offset = (unsigned)(p-pF);
			tmp.z = offset / xyPlane;

			offset -= tmp.z * xyPlane;
			tmp.y = offset / xLine;

			offset -= tmp.y * xLine;
			tmp.x = offset;

			surfacePoint.toMicronsFrom(tmp, res,off);
			++p;

			/* this is the closed-form of the algorithm:
			while (p != pL)
			{
				if (pxPredicate(*p))
				{
					//define surfacePoint from the (p-pF) offset...
				}
				++p;
			}
			*/

			//is the new point sufficiently far from the previous one?
			tmpB  = lastSurfacePoint;
			tmpB -= surfacePoint;
			tmpB.elemAbs();

			//"sufficiently far" means, per axis:
			//  is there a move?  if so, is it large enough?
		} while ((tmpB.x > 0 && tmpB.x < micronStep.x) ||
		         (tmpB.y > 0 && tmpB.y < micronStep.y) ||
		         (tmpB.z > 0 && tmpB.z < micronStep.z));

		lastSurfacePoint = surfacePoint;
		return true;
	}

private:
	const std::function<bool(const VT)>* pxPredicate;
	Vector3d<ST> micronStep;

	Vector3d<ST> res,off;
	size_t xLine,xyPlane;
	const VT* p,* pF,* pL;

	Vector3d<size_t> tmp;
	Vector3d<ST> lastSurfacePoint, tmpB;

	void theCommonInit(const i3d::Image3d<VT>& img)
	{
		res.fromI3dVector3d(img.GetResolution().GetRes());
		off.fromI3dVector3d(img.GetOffset());

		xLine = img.GetSizeX();
		xyPlane = xLine * img.GetSizeY();

		p = pF = img.GetFirstVoxelAddr();
		pL = pF + img.GetImageSize();

		lastSurfacePoint = -TOOFAR;
	}
};
#endif
