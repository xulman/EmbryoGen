#ifndef SURFACESAMPLERS_H
#define SURFACESAMPLERS_H

#include <math.h>

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
#endif
