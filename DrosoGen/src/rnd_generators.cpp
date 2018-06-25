#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <time.h>
#include <unistd.h>

#include "tools.h"
#include "rnd_generators.h"

/*
 * Based on Pierrre L'Ecuyer, http://www.iro.umontreal.ca/~lecuyer/,
 * well performing random number generators also available in the GSL are:
 *
 * gsl_rng_mt19937
 * gsl_rng_taus2
 *
 * In case, one would like to give them a try, change lines
 *
 *		rngHandle.rngState = gsl_rng_alloc(gsl_rng_default);
 * to, for example,
 *		rngHandle.rngState = gsl_rng_alloc(gsl_rng_mt19937);
 *
 * Both "super generators", probably, need no extra seeding.
 * They should do seed themselves somehow...
 * http://www.gnu.org/software/gsl/manual/html_node/Random-number-generator-algorithms.html
 */

/// re-seed if necessary (and init at all if necessary too)
void inline PossiblyReSeed(rndGeneratorHandle& rngHandle)
{
	if (rngHandle.usageCnt == UsageBeforeReseed)
	{
		//this is a bit dangerous in general to hide this test inside here,
		//but if world around is consistent... we are saving one 'if-test' per call
		if (rngHandle.rngState == NULL)
			rngHandle.rngState = gsl_rng_alloc(gsl_rng_default);

		//const unsigned long s = -1 * (int)time(NULL) * (int)getpid() * ((int)thrID+1) * 1000;
		const unsigned long s = (unsigned)(-1 * time(NULL) * getpid());
		gsl_rng_set(rngHandle.rngState,s);
		DEBUG_REPORT("randomness started with seed " << s);
	}
}


float GetRandomGauss(const float mean, const float sigma, rndGeneratorHandle& rngHandle)
{
	PossiblyReSeed(rngHandle);
	return ( (float)gsl_ran_gaussian(rngHandle.rngState, sigma) + mean );
}


float GetRandomUniform(const float A, const float B, rndGeneratorHandle& rngHandle)
{
	PossiblyReSeed(rngHandle);
	return ( (float)gsl_ran_flat(rngHandle.rngState, A,B) );
}


unsigned int GetRandomPoisson(const float mean, rndGeneratorHandle& rngHandle)
{
	PossiblyReSeed(rngHandle);
	return ( gsl_ran_poisson(rngHandle.rngState, mean) );
}
