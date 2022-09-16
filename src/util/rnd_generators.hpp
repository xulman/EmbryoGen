#pragma once

#include <gsl/gsl_rng.h>

struct rndGeneratorHandle {
	// constructor to create uninitialized yet valid handle
	rndGeneratorHandle(void) {
		reseedPeriod = 1000000;

		usageCnt = reseedPeriod; // will trigger seeding immediately
		rngState = nullptr;
	}

	// constructor to create uninitialized yet valid handle
	rndGeneratorHandle(const int _reseedPeriod) {
		reseedPeriod = _reseedPeriod;

		usageCnt = reseedPeriod; // will trigger seeding immediately
		rngState = nullptr;
	}

	~rndGeneratorHandle()
	{
		if (rngState)
			gsl_rng_free(rngState);
	}

	int reseedPeriod, usageCnt;
	gsl_rng* rngState;
};

/**
 * Generates random numbers that tend to form in Gaussian distribution
 * with given \e mean and \e sigma.
 *
 * \param[in] mean     	mean of the distribution
 * \param[in] sigma    	sigma of the distribution
 * \param[in] rngHandle	reference on the same underlying generator
 *
 * The purpose of the last parameter is that every client uses
 * its own handle when accessing random variables out of this generator.
 * If all clients use the same handle, the distribution they would
 * read-out from the generator might be biased (how much is a
 * randomly-and-or-systematically chosen subset still Gaussian distributed?).
 *
 * The function uses GSL random number generator:
 * http://www.gnu.org/software/gsl/manual/html_node/The-Gaussian-Distribution.html
 */
float GetRandomGauss(const float mean,
                     const float sigma,
                     rndGeneratorHandle& rngHandle);

/** The same as GetRandomGauss(...,rngHandle) but default handle is used.
    This may be used in non-critical applications. */
float GetRandomGauss(const float mean, const float sigma);

/**
 * Generates random numbers that tend to form in uniform/flat distribution
 * within given interval \e A and \e B.
 *
 * \param[in] A        	start of the interval
 * \param[in] B        	end of the interval
 * \param[in] rngHandle	reference on the same underlying generator
 *
 * The purpose of the last parameter is that every client uses
 * its own handle when accessing random variables out of this generator.
 * If all clients use the same handle, the distribution they would
 * read-out from the generator might be biased (how much is a
 * randomly-and-or-systematically chosen subset still Uniformly distributed?).
 *
 * The function uses GSL random number generator:
 * http://www.gnu.org/software/gsl/manual/html_node/The-Flat-_0028Uniform_0029-Distribution.html
 */
float GetRandomUniform(const float A,
                       const float B,
                       rndGeneratorHandle& rngHandle);

/** The same as GetRandomUniform(...,rngHandle) but default handle is used.
    This may be used in non-critical applications. */
float GetRandomUniform(const float A, const float B);

/**
 * Generates random numbers that tend to form in Poisson distribution
 * with given \e mean.
 *
 * \param[in] mean     	mean of the distribution
 * \param[in] rngHandle	reference on the same underlying generator
 *
 * The purpose of the last parameter is that every client uses
 * its own handle when accessing random variables out of this generator.
 * If all clients use the same handle, the distribution they would
 * read-out from the generator might be biased (how much is a
 * randomly-and-or-systematically chosen subset still Poisson distributed?).
 *
 * The function uses GSL random number generator:
 * http://www.gnu.org/software/gsl/manual/html_node/The-Poisson-Distribution.html
 */
unsigned int GetRandomPoisson(const float mean, rndGeneratorHandle& rngHandle);

/** The same as GetRandomPoisson(...,rngHandle) but default handle is used.
    This may be used in non-critical applications. */
unsigned int GetRandomPoisson(const float mean);
