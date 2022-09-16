#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <map>
#include <vector>

///define to obtain some debugging auxiliary outputs
//#define RNDWLK_DEBUG


/**
 * \ingroup toolbox
 *
 * A class responsible for generation of random walk
 */
template <typename FT>
class RandomWalk
{
  public:
	// ================= INITIALIZATION STUFF ================= 

	RandomWalk()
		: rnd_ForceNewSeedsAng(-1), rnd_ForceNewSeedsZ(-1), rnd_ForceNewSeedsDisp(-1)
	{
		//initiate rnd number generators
		rnd_StateAng=gsl_rng_alloc(gsl_rng_default);
		rnd_StateZ=gsl_rng_alloc(gsl_rng_default);
		rnd_StateDisp=gsl_rng_alloc(gsl_rng_default);
		if (!rnd_StateAng || !rnd_StateZ || !rnd_StateDisp)
throw report::rtError("Couldn't initialize random number generators.");

		unsigned long seed=(unsigned long)(-1 * time(NULL) * getpid());
		gsl_rng_set(rnd_StateAng,seed);
		gsl_rng_set(rnd_StateZ,seed+1);
		gsl_rng_set(rnd_StateDisp,seed+2);
		rnd_UseCounterAng=0;
		rnd_UseCounterZ=0;
		rnd_UseCounterDisp=0;

		//initiate rnd walk parameters
		wlk_displacementBias_x=wlk_displacementBias_y=wlk_displacementBias_z=0.;
		wlk_xy_heading=0.;
		wlk_z_limit=1.; //full 3D isotropy
		wlk_displacement_x=wlk_displacement_y=wlk_displacement_z=0.;
	}

	~RandomWalk()
	{
		//clear rnd number generators
		if (rnd_StateAng)  { gsl_rng_free(rnd_StateAng);  rnd_StateAng=NULL; }
		if (rnd_StateZ)    { gsl_rng_free(rnd_StateZ);    rnd_StateZ=NULL; }
		if (rnd_StateDisp) { gsl_rng_free(rnd_StateDisp); rnd_StateDisp=NULL; }
	}

	// ================= RND WALKING STUFF ================= 

	/**
	 * The map<angle,probability> describes probability distribution function
	 * of the walk turning angles (in radians), angles must be within [-3.14; 3.14],
	 * probability values must be non-negative.
	 *
	 * It can be either a histogram from a representant real walk or can be filled
	 * with theoretical distribution, see RandomWalk::UseWrappedCauchyDistribution().
	 *
	 * The values for unspecified angles will be interpolated from their "neighbors",
	 * it is assumed zero probability for angles less than -3.14 or more than +3.14.
	 *
	 * If an uniform distribution is provided, e.g. with RandomWalk::UseUniformDistribution(),
	 * the produced random walk will be isotropic (simulating Brownian motion), otherwise it
	 * will always be correlated to the previous move (simulating persistent walks).
	 *
	 * Note that a persistent time from walks simulated with this class can be calculated
	 * with this class only for the distributions generated with the function
	 * RandomWalk::UseWrappedCauchyDistribution(). For any other distribution, one must
	 * use some other SW (to fit theoretical persistence-driven curve to a measured MSD curve).
	 *
	 * This is the main input parameter of the walk. It must be, however, pre-processed
	 * with RandomWalk::PrepareRandomTurningField() for the walks to follow it.
	 *
	 * Note that it influences only the 2D portion (within the XY-plane) of a 3D walking.
	 * Note the (in principle) similar attribute RandomWalk::displacementStepSizeDistribution.
	 */
	std::map<FT,FT> turningAngleDistribution;

	/**
	 * Sets the RandomWalk::turningAngleDistribution such that generated random
	 * walks will simulate isotropic random walks.
	 *
	 * Note that it influences only the 2D portion (within the XY-plane) of a 3D walking.
	 */
	void UseUniformDistribution(void)
	{
		//relays on the interpolation in the RandomWalk::PrepareRandomTurningField()
		turningAngleDistribution.clear();
		turningAngleDistribution[-3.138]=1.;
		turningAngleDistribution[+3.138]=1.;
	}

	/**
	 * Sets the RandomWalk::turningAngleDistribution such that the observed
	 * distribution of turning angles will be more-or-less uniform (which
	 * is unfortunatelly not the case when RandomWalk::UseUniformDistribution()
	 * is used).
	 *
	 * Note that it influences only the 2D portion (within the XY-plane) of a 3D walking.
	 */
	void UseCorrectedUniformDistribution(void)
	{
		turningAngleDistribution.clear();
		for (int i=0; i < 180; ++i)
		{
			turningAngleDistribution[FT(i-180)/180.*3.14]=-18.0*(float(i)/180.) +31.0;
			turningAngleDistribution[    FT(i)/180.*3.14]= 18.0*(float(i)/180.) +13.0;
		}
	}

	/**
	 * Sets the RandomWalk::turningAngleDistribution such that generated random
	 * walks will simulate correlated random walks. The underlying turning angle
	 * distribution will be due to the wrapped Cauchy (circular) distribution.
	 *
	 * \parameter[in] r	Value from [0. , 1.).
	 *
	 * In fact, the parameter \e r is the mean cosine angle of the turning angle
	 * distribution. Setting it to \e r=0 yields uniform resolution (and hence
	 * isotropic random walks). The closer to 1 it is, the more straight the walks
	 * will be (and hence less random).
	 *
	 * Note that it influences only the 2D portion (within the XY-plane) of a 3D walking.
	 */
	void UseWrappedCauchyDistribution(const FT r)
	{
		turningAngleDistribution.clear();
		for (int i=0; i <= 62; ++i)
		{
			const FT angle=FT(i-31)*0.1013;
			turningAngleDistribution[angle]=0.159 * ( (1.-r*r) / (1. +r*r -2.*r*cos(angle)) );
		}
	}

	/**
	 * Sets the RandomWalk::turningAngleDistribution such that generated random
	 * walks will simulate correlated random walks. The underlying turning angle
	 * distribution will be due to the wrapped Gauss/normal (circular) distribution.
	 *
	 * \parameter[in] sigma		Basically any non-negative value.
	 *
	 * In fact, the parameter \e sigme should be reasonable considering that the
	 * span of turning angles will be [-3*sigma,+3*sigma]. Very large sigma should
	 * produce something similar to isotropic random walk. For the mean cosine angle
	 * r it holds r=exp(-\e sigma*\e sigma/2). And, sigma=sqrt(-2*ln(r)).
	 *
	 * Note that it influences only the 2D portion (within the XY-plane) of a 3D walking.
	 */
	void UseWrappedNormalDistribution(const FT sigma)
	{
		turningAngleDistribution.clear();
		for (int i=0; i <= 62; ++i)
		{
			const FT angle=FT(i-31)*0.1013;
			turningAngleDistribution[angle]=1./(sigma*2.506) * (
			        exp(-0.5*(angle-6.28)*(angle-6.28) / (sigma*sigma))
			      + exp(-0.5*angle*angle               / (sigma*sigma))
			      + exp(-0.5*(angle+6.28)*(angle+6.28) / (sigma*sigma)) );
		}
	}


  protected:
	/**
	 * This attribute is used for drawing random turning angles and is hence important!
	 *
	 * It is a helper array to provide turning angles: a uniform random number generator
	 * chooses index to this array and the value found there is the next turning angle.
	 * Clearly, the histogram of angle values present in this array should be the same
	 * as is given in the RandomWalk::turningAngleDistribution attribute. For convenience,
	 * use the RandomWalk::PrepareRandomTurningField() method to fill this attribute.
	 *
	 * The array should be such long that every "discrete angle" (assuming sampling
	 * rate of 1/6.28 radians (1 deg)) whose probability is non-zero should be present
	 * in the array.
	 *
	 * Note that it influences only the 2D portion (within the XY-plane) of a 3D walking.
	 */
	std::vector<FT> randomTurningField;

  public:
	/**
	 * This updates the RandomWalk::randomTurningField
	 * from the RandomWalk::turningAngleDistribution.
	 *
	 * Since the latter may sample the interval [-3.14; 3.14] arbitrarily
	 * while this function considers angular stepping as described in the
	 * RandomWalk::randomTurningField, the probabilities of angles missing
	 * in the distribution are "guessed" using linear interpolation.
	 * It is assumed that edge values pose both zero (0) probability, unless
	 * the RandomWalk::turningAngleDistribution dictates otherwise.
	 *
	 * Note that it influences only the 2D portion (within the XY-plane) of a 3D walking.
	 * Note the (in principle) similar method RandomWalk::PrepareRandomStepSizeField().
	 */
	void PrepareRandomTurningField(void)
	{
		randomTurningField.clear();

		//first, resample the distribution function with fine stepping
		//second, calculate Sum of the probabilities and determine reasonable
		//  stretch Factor such that the randomTurningField is not way too long,
		//  that is the Sum * Factor is circa 10000
		//third, fill this field: insert/every angle into the array
		//  this number times: its probability times Factor
		//  (this is similar to histogram equalization...)

		//first:
		//create and init the local resampled distribution 
		FT distribution[361];
		for (int i=0; i < 361; ++i) distribution[i]=-1.;
		distribution[0]=distribution[360]=0.;

#ifdef RNDWLK_DEBUG
		std::ofstream file1("turnAngDist_coarse.dat");
#endif
		//fill it where we can
		typename std::map<FT,FT>::const_iterator it=turningAngleDistribution.begin();
		while (it != turningAngleDistribution.end())
		{
			if (it->first >= -3.14 && it->first <= 3.14)
			{
				distribution[(int)round((it->first+3.14)*360./6.28)]=it->second;
#ifdef RNDWLK_DEBUG
				file1 << it->first << "\t" << it->second << "\n";
#endif
			}
			++it;
		}
#ifdef RNDWLK_DEBUG
		file1.close();
#endif

		//search for uninitialized values and calculate them
		int lastLeftInited=0;
		int lastRightInited=1;
		while (lastRightInited < 361 && distribution[lastRightInited] == -1.) ++lastRightInited;
		//note that distribution[360] != -1. (which renders the first condition useless)

		//sum over all probabilities...
		FT Sum=distribution[0]+distribution[360];

		for (int i=1; i < 360; ++i)
		{
			if (distribution[i] == -1.)
			{
				//found uninitialized angle, interpolate from lastLeft and lastRight ones
				distribution[i]=FT(i-lastLeftInited)/FT(lastRightInited-lastLeftInited)
				                *(distribution[lastRightInited]-distribution[lastLeftInited])
									 +distribution[lastLeftInited];
			}
			else
			{
				//found initialized one, move the boundaries
				lastLeftInited=i; //should be also equal to lastRightInited
				lastRightInited=i+1;
				while (lastRightInited < 361 && distribution[lastRightInited] == -1.) ++lastRightInited;
			}

			Sum+=distribution[i];
		}

#ifdef RNDWLK_DEBUG
		//debug: print out the fine-sampled distribution
		std::ofstream file2("turnAngDist_fine.dat");
		for (int i=0; i < 361; ++i)
			file2 << 3.14*FT(i-180)/180. << "\t" << distribution[i] << "\n";
		file2.close();
#endif

		//second:
		const FT Factor=10000. / Sum;

		//third:
#ifdef RNDWLK_DEBUG
		std::ofstream file3("turnAngDist_rndFieldHistogram.dat");
#endif
		for (int i=0; i < 361; ++i)
		{
			const int length=(int)floor(distribution[i]*Factor);
			for (int j=0; j < length; ++j)
				randomTurningField.push_back(3.14*FT(i-180)/180.);
#ifdef RNDWLK_DEBUG
			file3 << 3.14*FT(i-180)/180. << "\t" << length << "\n";
#endif
		}
#ifdef RNDWLK_DEBUG
		file3.close();
#endif
	}


	/**
	 * The map<stepSize,probability> describes probability distribution function
	 * of the walk displacement steps (in whatever length unit, e.g. microns),
	 * the stepSize must not be negative, probability values must be non-negative too.
	 *
	 * This attribute can be typically filled from a histogram from a representant
	 * real walk, or can be filled with some theoretical distribution.
	 *
	 * The values for unspecified angles will be interpolated from their "neighbors",
	 * it is assumed zero probability for steps less than 0.0. The "right" limit
	 * (on maximum displacement size) must be explicitly given by setting it to zero
	 * probability: \e displacementStepSizeDistribution[maxStepSize]=0.f;
	 *
	 * This is the second main input parameter of the walk. It must be, however,
	 * pre-processed with RandomWalk::PrepareRandomStepSizeField() for the walks
	 * to follow it.
	 *
	 * Note the (in principle) similar attribute RandomWalk::turningAngleDistribution.
	 */
	std::map<FT,FT> displacementStepSizeDistribution;

	/**
	 * Attempts to create a single-peaked distribution of available displacement
	 * step sizes.
	 *
	 * \param[in] dispSize		the position of the peak
	 */
	void UseFixedDisplacementStep(const FT dispSize)
	{
		displacementStepSizeDistribution.clear();
		displacementStepSizeDistribution[0.]=0.;
		displacementStepSizeDistribution[dispSize-1.0]=0.;
		displacementStepSizeDistribution[dispSize-0.2]=0.;
		displacementStepSizeDistribution[dispSize-0.1]=1.;
		displacementStepSizeDistribution[dispSize+0.1]=1.;
		displacementStepSizeDistribution[dispSize+0.2]=0.;
		displacementStepSizeDistribution[dispSize+1.0]=0.;
	}

	void UseNormalDistDisplacementStep(const FT mean,const FT sigma)
	{
		displacementStepSizeDistribution.clear();
		int intStart=std::max(int((mean-3.f*sigma)*10.f),0);
		int intStop =std::max(int((mean+3.f*sigma)*10.f),intStart);
		for (int i=intStart; i <= intStop; ++i)
		{
			const FT step=FT(i)/10.f;
			displacementStepSizeDistribution[step]= 1./(sigma*2.506)
			      * exp(-0.5*(step-mean)*(step-mean) / (sigma*sigma));
		}
	}


  protected:
	/**
	 * This attribute is used for drawing random displacement step sizes and is hence important!
	 *
	 * It is a helper array to provide step sizes: a uniform random number generator
	 * chooses index to this array and the value found there is the next step size.
	 * Clearly, the histogram of step sizes values present in this array should be
	 * the same as is given in the RandomWalk::displacementStepSizeDistribution
	 * attribute. For convenience, use the RandomWalk::PrepareRandomStepSizeField()
	 * method to fill this attribute.
	 *
	 * The array should be such long that every "discrete step" (assuming user
	 * given sampling rate) whose probability is non-zero should be present
	 * in the array.
	 */
	std::vector<FT> randomStepSizeField;

  public:
	/**
	 * This updates the RandomWalk::randomStepSizeField
	 * from the RandomWalk::displacementStepSizeDistribution.
	 *
	 * Since the latter may sample the interval [0.0; some_max] arbitrarily
	 * while this function considers given user stepping, the probabilities of step
	 * sizes missing in the distribution are "guessed" using linear interpolation.
	 * It is assumed that right edge value poses zero (0) probability.
	 *
	 * \param[in] stepSampling	user given granularity of step sizes
	 *
	 * The granularity should be fine such that the RandomWalk::displacementStepSizeDistribution
	 * will not tend to aggregate its sampling nodes due to overly large
	 * \e stepSampling -- this method is not accommodated for that.
	 *
	 * Note the (in principle) similar method RandomWalk::PrepareRandomTurningField().
	 */
	void PrepareRandomStepSizeField(const FT stepSampling=1.)
	{
		//see the RandomWalk::PrepareRandomTurningField() for some more comments...
		if (stepSampling <= 0.)
throw report::rtError("The stepSampling parameter must be strictly positive.");

		randomStepSizeField.clear();

		//first:
		//create and init the local resampled distribution 
		//for which we need to determine the maximum value
		typename std::map<FT,FT>::const_iterator it
		   =displacementStepSizeDistribution.end();
		--it;

		//if the last stored probability is not zero, inject ours...
		if (it->second != 0.)
		{
			displacementStepSizeDistribution[std::max(it->first,FT(0.))+stepSampling]=0.;
			++it;
		}

		//create and init the local resampled distribution 
		const int distLength=(int)round(it->first/stepSampling) +1;
		FT distribution[distLength];
		for (int i=0; i < distLength; ++i) distribution[i]=-1.;
		distribution[0]=distribution[distLength-1]=0.;

#ifdef RNDWLK_DEBUG
		std::ofstream file1("stepSizeDist_coarse.dat");
#endif
		//fill it where we can
		it=displacementStepSizeDistribution.begin();
		while (it != displacementStepSizeDistribution.end())
		{
			if (it->first >= 0.0)
			{
				distribution[(int)round(it->first/stepSampling)]=it->second;
#ifdef RNDWLK_DEBUG
				file1 << it->first << "\t" << it->second << "\n";
#endif
			}
			++it;
		}
#ifdef RNDWLK_DEBUG
		file1.close();
#endif

		//search for uninitialized values and calculate them
		int lastLeftInited=0;
		int lastRightInited=1;
		while (lastRightInited < distLength && distribution[lastRightInited] == -1.) ++lastRightInited;
		//note that distribution[distLength-1] != -1. (which renders the first condition useless)

		//sum over all probabilities...
		FT Sum=distribution[0]+distribution[distLength-1];

		for (int i=1; i < distLength-1; ++i)
		{
			if (distribution[i] == -1.)
			{
				//found uninitialized value, interpolate from lastLeft and lastRight ones
				distribution[i]=FT(i-lastLeftInited)/FT(lastRightInited-lastLeftInited)
				                *(distribution[lastRightInited]-distribution[lastLeftInited])
									 +distribution[lastLeftInited];
			}
			else
			{
				//found initialized one, move the boundaries
				lastLeftInited=i; //should be also equal to lastRightInited
				lastRightInited=i+1;
				while (lastRightInited < distLength && distribution[lastRightInited] == -1.) ++lastRightInited;
			}

			Sum+=distribution[i];
		}

#ifdef RNDWLK_DEBUG
		//debug: print out the fine-sampled distribution
		std::ofstream file2("stepSizeDist_fine.dat");
		for (int i=0; i < distLength; ++i)
			file2 << i*stepSampling << "\t" << distribution[i] << "\n";
		file2.close();
#endif

		//second:
		const FT Factor=10000. / Sum;

		//third:
#ifdef RNDWLK_DEBUG
		std::ofstream file3("stepSizeDist_rndFieldHistogram.dat");
#endif
		for (int i=0; i < distLength; ++i)
		{
			const int length=(int)floor(distribution[i]*Factor);
			for (int j=0; j < length; ++j)
				randomStepSizeField.push_back(i*stepSampling);
#ifdef RNDWLK_DEBUG
			file3 << i*stepSampling << "\t" << length << "\n";
#endif
		}
#ifdef RNDWLK_DEBUG
		file3.close();
#endif
	}


	///Set bias vector to use.
	void SetBiasVector(const FT x,const FT y,const FT z=0.)
	{
		wlk_displacementBias_x=x; wlk_displacementBias_y=y; wlk_displacementBias_z=z;
report::debugMessage(fmt::format("new Bias vector: ({},{},{}) in microns" , wlk_displacementBias_x, wlk_displacementBias_y, wlk_displacementBias_z));
	}

	///Get currently used bias 2D vector.
	void GetBiasVector(FT& x,FT& y) const
	{ x=wlk_displacementBias_x; y=wlk_displacementBias_y; }

	///Get currently used bias 3D vector.
	void GetBiasVector(FT& x,FT& y,FT& z) const
	{ x=wlk_displacementBias_x; y=wlk_displacementBias_y; z=wlk_displacementBias_z; }

	///Indicates whether simulated walks are biased.
	bool IsWalkBiased(void) const
	{ return (wlk_displacementBias_x != 0.
	      || wlk_displacementBias_y != 0. || wlk_displacementBias_z != 0.); }


	///Sets the current 2D heading; should be used only at initialization
	void ResetXYHeading(FT newHeading)
	{
		//shift to the kanonical interval <0,2PI>
		while (newHeading < 0.) newHeading+=6.28318;
		while (newHeading > 6.28318) newHeading-=6.28318;

		wlk_xy_heading=newHeading;
report::debugMessage(fmt::format("new XY heading angle: {} radians" , wlk_xy_heading));
	}


	/**
	 * Sets the 3D walk anisotropy (only in the direction of the z-axis).
	 * The value must be within 0 and 1 (inclusive) interval. Value of 0 enforces
	 * no movement along the z-axis, and value of 1 allows for a movement, e.g.,
	 * only along the z-axis (no xy-movement).
	 */
	void SetZstepLimit(FT zLimit)
	{
		//make sure the limit is a valid value
		if (zLimit < 0.) zLimit=0.;
		if (zLimit > 1.) zLimit=1.;

		//set it then
		wlk_z_limit=zLimit;
report::debugMessage(fmt::format("new Z anisotropy value: {} [no units]" , wlk_z_limit));
	}

	///Returns the current 3D walk z-axis anisotropy.
	FT GetZstepLimit(void) const
	{ return (wlk_z_limit); }


	/**
	 * Indication whether all data structures are prepared.
	 * Of course, it does not investigate the content/meaningfulness
	 * of the data.
	 */
	bool ReadyToStartSimulation(void) const
	{
		return (randomTurningField.size() > 0
		     && randomStepSizeField.size() > 0);
	}


	///Do one step of the 2D random walk
	void DoOneStep2D(void)
	{
		//choose random turning angle from the underlying distribution
		//(which is represented in this helper field)
		const FT deltaHeading=randomTurningField[
		          (int)floor(rnd_GetValueAng(0.,randomTurningField.size()-0.001)) ];

		//update the azimuth (absolute heading)
		wlk_xy_heading+=deltaHeading;

		//choose random step size from the underlying distribution
		const FT dispSize=randomStepSizeField[
		          (int)floor(rnd_GetValueDisp(0.,randomStepSizeField.size()-0.001)) ];

		//update the (normalized) position displacement "vector"
		wlk_displacement_x=dispSize*cos(wlk_xy_heading);
		wlk_displacement_y=dispSize*sin(wlk_xy_heading);

		//add the bias...
		wlk_displacement_x+=wlk_displacementBias_x;
		wlk_displacement_y+=wlk_displacementBias_y;
	}

	///Do one step of the 2D random walk and return fresh new generated displacement
	void DoOneStep2D(FT& x,FT& y)
	{
		DoOneStep2D();
		x=wlk_displacement_x; y=wlk_displacement_y;
	}


	///Do one step of the 3D random walk
	void DoOneStep3D(void)
	{
		//choose random 2D turning angle from the underlying distribution
		//(which is represented in this helper field)
		const FT deltaHeading=randomTurningField[
		          (int)floor(rnd_GetValueAng(0.,randomTurningField.size()-0.001)) ];

		//update the azimuth (absolute heading)
		wlk_xy_heading+=deltaHeading;

		//choose random z-value;
		//it will be a number from interval [-wlk_z_limit ; wlk_z_limit]
		const FT z_step=rnd_GetValueZ();

		//dumping in order to provide uniform "sphere point picking"
		//http://mathworld.wolfram.com/SpherePointPicking.html
		const FT z_xy_dumping=sin(acos(z_step));

		//choose random step size from the underlying distribution
		const FT dispSize=randomStepSizeField[
		          (int)floor(rnd_GetValueDisp(0.,randomStepSizeField.size()-0.001)) ];

		//update the (normalized) position displacement "vector"
		wlk_displacement_x=dispSize*cos(wlk_xy_heading)*z_xy_dumping;
		wlk_displacement_y=dispSize*sin(wlk_xy_heading)*z_xy_dumping;
		wlk_displacement_z=dispSize*z_step;

		//add the bias...
		wlk_displacement_x+=wlk_displacementBias_x;
		wlk_displacement_y+=wlk_displacementBias_y;
		wlk_displacement_z+=wlk_displacementBias_z;
	}

	///Do one step of the 3D random walk and return fresh new generated displacement
	void DoOneStep3D(FT& x,FT& y,FT& z)
	{
		DoOneStep3D();
		x=wlk_displacement_x; y=wlk_displacement_y; z=wlk_displacement_z;
	}


	///Get the last 2D generated displacement; does no simulation (see RandomWalk::DoOneStep2D())
	void GetLastWalkedDisplacement(FT& dx,FT& dy) const
	{ dx=wlk_displacement_x; dy=wlk_displacement_y; }

	///Get the last 3D generated displacement; does no simulation (see RandomWalk::DoOneStep3D())
	void GetLastWalkedDisplacement(FT& dx,FT& dy,FT& dz) const
	{ dx=wlk_displacement_x; dy=wlk_displacement_y; dz=wlk_displacement_z; }


  protected:
	///Current bias vector (if _WithBias mode is used)
	FT wlk_displacementBias_x,wlk_displacementBias_y,wlk_displacementBias_z;

	///Current heading/azimuth (in radians, in the XY-plane) of this random walk.
	FT wlk_xy_heading;

	///Current limit for the z-values in the 3D walk. This imposses anisotropy of the 3D walk. Value must be in [0,1].
	FT wlk_z_limit;

	///last position coordinate update
	FT wlk_displacement_x,wlk_displacement_y,wlk_displacement_z;


	// ================= RND GENERATORS STUFF ================= 

	///helper random number generator: Flat/uniform distribution within [\e A,\e B]
	FT rnd_GetValueAng(const FT A, const FT B)
	{
		if (rnd_UseCounterAng == rnd_ForceNewSeedsAng)
		{
			unsigned long seed=(unsigned long)(-1 * time(NULL) * getpid());
			gsl_rng_set(rnd_StateAng,seed);
			rnd_UseCounterAng=0;
		}

		++rnd_UseCounterAng;
		return ( gsl_ran_flat(rnd_StateAng, A,B) );
	}

	///helper random number generator: Flat/uniform distribution within [ -this->wlk_z_limit, this->wlk_z_limit ]
	FT rnd_GetValueZ(void)
	{
		if (rnd_UseCounterZ == rnd_ForceNewSeedsZ)
		{
			unsigned long seed=(unsigned long)(-1 * time(NULL) * getpid());
			gsl_rng_set(rnd_StateZ,seed);
			rnd_UseCounterZ=0;
		}

		++rnd_UseCounterZ;
		return ( gsl_ran_flat(rnd_StateZ, -wlk_z_limit,wlk_z_limit) );
	}

	///helper random number generator: Flat/uniform distribution within [\e A,\e B]
	FT rnd_GetValueDisp(const FT A, const FT B)
	{
		if (rnd_UseCounterDisp == rnd_ForceNewSeedsDisp)
		{
			unsigned long seed=(unsigned long)(-1 * time(NULL) * getpid());
			gsl_rng_set(rnd_StateDisp,seed);
			rnd_UseCounterDisp=0;
		}

		++rnd_UseCounterDisp;
		return ( gsl_ran_flat(rnd_StateDisp, A,B) );
	}

	///helper random number generator: generator states
	gsl_rng *rnd_StateAng, *rnd_StateZ, *rnd_StateDisp;

	///helper random number generator: use counters (to enforce re-seeding)
	long rnd_UseCounterAng, rnd_UseCounterZ, rnd_UseCounterDisp;

	///helper random number generator: how many rnd numbers before re-seeding
	const long rnd_ForceNewSeedsAng, rnd_ForceNewSeedsZ, rnd_ForceNewSeedsDisp;
};
