#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <time.h>
#include <iostream>

#include "params.h"
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
 *		randState = gsl_rng_alloc(gsl_rng_default);
 * to, for example,
 *		randState = gsl_rng_alloc(gsl_rng_mt19937);
 *
 * Both "super generator", probably, need no extra seeding.
 * They should do seed themselves somehow...
 * http://www.gnu.org/software/gsl/manual/html_node/Random-number-generator-algorithms.html
 */

float GetRandomGauss(const float mean, const float sigma) {
	static gsl_rng *randState=NULL;

	//do we need to init random generator?
	if (randState == NULL) {
		//yes, we do:
		//create instance of the generator and seed it
		randState = gsl_rng_alloc(gsl_rng_default);
		unsigned long s=-1 * (int) time(NULL);
		DEBUG_REPORT("GetRandomGauss(): randomness started with seed " << s);
		gsl_rng_set(randState,s);
	}

	return ( (float)gsl_ran_gaussian(randState, sigma) + mean );
}


float GetRandomUniform(const float A, const float B) {
	static gsl_rng *randState=NULL;

	//do we need to init random generator?
	if (randState == NULL) {
		//yes, we do:
		//create instance of the generator and seed it
		randState = gsl_rng_alloc(gsl_rng_default);
		unsigned long s=-1 * (int) time(NULL);
		DEBUG_REPORT("GetRandomUniform(): randomness started with seed " << s);
		gsl_rng_set(randState,s);
	}

	return ( (float)gsl_ran_flat(randState, A,B) );
}


unsigned int GetRandomPoisson(const float mean) {
	static gsl_rng *randState=NULL;

	//do we need to init random generator?
	if (randState == NULL) {
		//yes, we do:
		//create instance of the generator and seed it
		randState = gsl_rng_alloc(gsl_rng_default);
		unsigned long s=-1 * (int) time(NULL);
		DEBUG_REPORT("GetRandomPoisson(): randomness started with seed " << s);
		gsl_rng_set(randState,s);
	}

	return ( gsl_ran_poisson(randState, mean) );
}


template <typename MT>
void SuggestBrownianVector(Vector3d<MT> &v, const MT step)
{
	 // wrapper for low level function
	 SuggestBrownianVector(v, Vector3d<MT>(step, step, step));
}

template <typename MT>
void SuggestBrownianVector(Vector3d<MT> &v, const Vector3d<MT> step)
{
	//1.6f is a correction coefficient such that the mean size
	//of the generated vectors is 'step', variance of generated
	//vectors will cca 0.42 the mean - that's just a fact :-)
	/*v.x=static_cast<MT>((float)step/1.6f * GetRandomGauss(0,1));
	v.y=static_cast<MT>((float)step/1.6f * GetRandomGauss(0,1));
	v.z=static_cast<MT>((float)step/1.6f * GetRandomGauss(0,1));*/

	// for 2x bigger cells, we will force the Brownian vector to be stronger
	v.x=static_cast<MT>(1.0f * (float)step.x/1.6f * GetRandomGauss(0,1));
	v.y=static_cast<MT>(1.0f * (float)step.y/1.6f * GetRandomGauss(0,1));
	v.z=static_cast<MT>(1.0f * (float)step.z/1.6f * GetRandomGauss(0,1));
}

template void SuggestBrownianVector(Vector3d<float> &v, const float step);
template void SuggestBrownianVector(Vector3d<float> &v, const Vector3d<float> step);

