#ifndef GTGEN_RND_GENERATORS_H
#define GTGEN_RND_GENERATORS_H

#include "params.h"

/**
 * Generates random numbers that tend to form in Gaussian distribution
 * with given \e mean and \e sigma.
 *
 * \param[in] mean	mean of the distribution
 * \param[in] sigma	sigma of the distribution
 *
 * The function uses GSL random number generator:
 * http://www.gnu.org/software/gsl/manual/html_node/The-Gaussian-Distribution.html
 */
float GetRandomGauss(const float mean, const float sigma);

/**
 * Generates random numbers that tend to form in uniform/flat distribution
 * within given interval \e A and \e B.
 *
 * \param[in] A		start of the interval
 * \param[in] B		end of the interval
 *
 * The function uses GSL random number generator:
 * http://www.gnu.org/software/gsl/manual/html_node/The-Flat-_0028Uniform_0029-Distribution.html
 */
float GetRandomUniform(const float A, const float B);

/**
 * Generates random numbers that tend to form in Poisson distribution
 * with given \e mean.
 *
 * \param[in] mean	mean of the distribution
 *
 * The function uses GSL random number generator:
 * http://www.gnu.org/software/gsl/manual/html_node/The-Poisson-Distribution.html
 */
unsigned int GetRandomPoisson(const float mean);

/**
 * Generates random 3D velocity vector \e v.
 *
 * When called iteratively, the vectors should yield a "Brownian motion"
 * (if evaluated, e.g., with the MSD curve).
 *
 * \param[out] v	random velocity vector
 * \param[in] step	expected mean length of the velocity vector
 *
 * The generated vector \e v, before it is returned, is multiplied with the
 * \e step parameter. The \e step parameter has a default value of 1.
 *
 * \note The parameter \e step is, basically, nothing else than some
 * multiplicative weighting factor. However, testing this function revealed
 * that it produces vectors of mean magnitude \e step with variance
 * cca. 0.42 times \e step.
 *
 * \note In theory, using this function you can also generate a "constrained
 * motion" (when you won't let the dot to move too far from some centre point)
 * as well as "directional motion" (when you systematically add certain fixed
 * vector which "directionally" biases the motion to its direction).
 */
template <typename MT>
void SuggestBrownianVector(Vector3d<MT> &v, const Vector3d<MT> step=Vector3d<MT>(1,1,1));

/// wrapper for low level function
template <typename MT>
void SuggestBrownianVector(Vector3d<MT> &v, const MT step=1);


/**
 * Based on an observation of a few real videos, we have designed
 * this multi-modal function which states how likely is the given
 * rotation in the interval [-35:35] in degs to happen. Use Gnuplot:
 * 
 * plot [-35:35] (35-abs(x))/35*0.6  *0.5*cos(x*0.571198)+ (35-abs(x))/35  *(0.5+(1-0.6)/2);
 * 
 * where 35 is the range in one direction,
 * 0.571198 converts deg to rad such that 11deg is 2PI,
 * 0.6 shrinks the curve such that it does not touch zero,
 * (35-abs(x)/35 forces the curve to approach zero at tails
 *
 * \return Returns angle in radians.
 */
float GetRandomRotationMultimodal(void);

#endif
