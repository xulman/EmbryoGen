#ifndef UTIL_SYNTHOSCOPY_SNR_H
#define UTIL_SYNTHOSCOPY_SNR_H

/**********************************************************************
*
* finalpreview.h
*
* This file is part of MitoGen
*
* Copyright (C) 2013-2016 -- Centre for Biomedical Image Analysis (CBIA)
* http://cbia.fi.muni.cz/
*
* MitoGen is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* MitoGen is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with MitoGen. If not, see <http://www.gnu.org/licenses/>.
*
* Author: David Svoboda and Vladimir Ulman
*
***********************************************************************/

#include <i3d/image3d.h>

namespace mitogen
{

/**
 * \ingroup toolbox
 *
 * Computes the Signal-to-Noise Ratio (SNR) of the input image \e img
 * given that signal voxels are those within the mask \e mask.
 * Voxels outside the mask are considered as background.
 *
 * The SNR is computed as:
 * \verbatim
 * (avg(fg region) - avg(bg region)) / std_dev(bg region)
 * \endverbatim
 *
 * This function, however, uses two-pass algorithms for 'compensated'
 * computations of both the mean and variance. It should be more accurate.
 *
 * For the mean, Kahan summation alg. is used [1].
 * \verbatim
 * mean=0
 * compensation=0	//a running compensation for lost low-order bits
 *
 * for d in data_to_be_summed
 *   d_compensated=d-compensation
 *   sum=mean+d_compensated							//this potentially loses low-order bits
 *   compensation=(sum-mean) - d_compensated		//recovers what's been lost and adds it in the next round,
 *   															//algebraically, 'compensation' should be zero
 *   mean=sum
 * endfor
 * \endverbatim
 *
 * For the variance (square of the standard deviation) the eq. (1.4) [2],\
 * the prof. Bjorek's alg., is used.
 * \verbatim
 * d is data_to_be_summed, mean is mean of this data:
 * variance=[ sum(d-mean)^2 - (sum(d-mean))^2/N ] / N
 * \endverbatim
 * The Kahan summation is applied for both summations as well.
 *
 * Literature:
 *
 * [1] Kahan, William (January 1965), "Further remarks on reducing truncation errors",
 * Communications of the ACM 8 (1): 40, doi:10.1145/363707.363723.
 * http://en.wikipedia.org/wiki/Kahan_summation_algorithm
 *
 * [2] Chan, Tony F.; Golub, Gene H.; LeVeque, Randall J. (1979),
 * "Updating Formulae and a Pairwise Algorithm for Computing Sample Variances.",
 * Technical Report STAN-CS-79-773, Department of Computer Science, Stanford University.
 * ftp://reports.stanford.edu/pub/cstr/reports/cs/tr/79/773/CS-TR-79-773.pdf
 *
 * \param[in] img		image of which SNR is to be computed
 * \param[in] mask	corresponding mask image
 *
 * \return Returns the computed SNR.
 */
template <class MV, class PV>
double ComputeSNR(i3d::Image3d<PV> const &img,
                  i3d::Image3d<MV> const &mask);

}
#endif
