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

#include "../report.hpp"
#include <fmt/core.h>
#include <i3d/image3d.h>

namespace mitogen {

template <class MV, class PV>
double ComputeSNR(i3d::Image3d<PV> const& img, i3d::Image3d<MV> const& mask) {
	if (img.GetSize() != mask.GetSize())
		throw report::rtError("Phantom and mask images do not match in size.");

	// helper sums for the fg and bg regions
	double sumFg = 0.f, compFg = 0.f;
	double sumBg = 0.f, compBg = 0.f;

	// sizes
	unsigned long sizeFg = 0;
	unsigned long sizeBg = 0;

	// running pointers
	const PV* pI = img.GetFirstVoxelAddr();
	const MV* pM = mask.GetFirstVoxelAddr();
	PV const* LastpI = img.GetFirstVoxelAddr() + img.GetImageSize();

	// the first pass, computation of the means:
	//'sum' will accumulate for mean, 'comp' will do the compensation job

	for (; pI != LastpI; ++pI, ++pM) {
		// first, convert...
		const double tmp = static_cast<double>(*pI);

		if (*pM) {
			// foreground
			const double tmpC = tmp - compFg;
			const double tmpS = sumFg + tmpC;
			compFg = (tmpS - sumFg) - tmpC;
			sumFg = tmpS;

			++sizeFg;
		} else {
			// background
			const double tmpC = tmp - compBg;
			const double tmpS = sumBg + tmpC;
			compBg = (tmpS - sumBg) - tmpC;
			sumBg = tmpS;

			++sizeBg;
		}
	}

	const double meanFg = sumFg / (double)sizeFg;
	const double meanBg = sumBg / (double)sizeBg;

	// the second pass, computation of the variances:
	// reset vars for first-order accumulations
	sumFg = 0.f;
	compFg = 0.f;
	sumBg = 0.f;
	compBg = 0.f;

	// prepare vars for squared (second-order) accumulations
	double sum2Fg = 0.f, comp2Fg = 0.f;
	double sum2Bg = 0.f, comp2Bg = 0.f;

	// reset the running pointers
	pI = img.GetFirstVoxelAddr();
	pM = mask.GetFirstVoxelAddr();

	for (; pI != LastpI; ++pI, ++pM) {
		// first, convert...
		const double tmp = static_cast<double>(*pI);

		if (*pM) {
			// foreground
			/* naive version, btw: shouldn't we do Kahan here as well?
			sumFg+=(tmp-meanFg);
			sum2Fg+=(tmp-meanFg) * (tmp-meanFg);
			*/
			double tmpC = (tmp - meanFg) - compFg;
			double tmpS = sumFg + tmpC;
			compFg = (tmpS - sumFg) - tmpC;
			sumFg = tmpS;

			tmpC = (tmp - meanFg) * (tmp - meanFg) - comp2Fg;
			tmpS = sum2Fg + tmpC;
			comp2Fg = (tmpS - sum2Fg) - tmpC;
			sum2Fg = tmpS;
		} else {
			// background
			/* naive version, btw: shouldn't we do Kahan here as well?
			sumBg+=(tmp-meanBg);
			sum2Bg+=(tmp-meanBg) * (tmp-meanBg);
			*/
			double tmpC = (tmp - meanBg) - compBg;
			double tmpS = sumBg + tmpC;
			compBg = (tmpS - sumBg) - tmpC;
			sumBg = tmpS;

			tmpC = (tmp - meanBg) * (tmp - meanBg) - comp2Bg;
			tmpS = sum2Bg + tmpC;
			comp2Bg = (tmpS - sum2Bg) - tmpC;
			sum2Bg = tmpS;
		}
	}

	const double devFg =
	    sqrt((sum2Fg - sumFg * sumFg / (double)sizeFg) / (double)sizeFg);
	const double devBg =
	    sqrt((sum2Bg - sumBg * sumBg / (double)sizeBg) / (double)sizeBg);

	const double SNR = (meanFg - meanBg) / devBg;

	report::debugMessage(fmt::format(
	    "fg / bg volume ratio: {}, fg constitues {}% of the image volume",
	    (double)sizeFg / (double)sizeBg,
	    (double)sizeFg / (double)mask.GetImageSize() * 100.f));
	report::debugMessage(
	    fmt::format("fg signal is {} +- {}, bg signal is {} +- {}", meanFg,
	                devFg, meanBg, devBg));
	report::debugMessage(fmt::format("SNR={}, CR={}", SNR, meanFg / meanBg));

	// tagged and formated for machine processing of the SNR related data...
	report::message(
	    fmt::format("SNR {} {} {} {} {}", SNR, meanFg, meanBg, devFg, devBg));

	return (SNR);
}

//
// explicit instantiations (just one, for now...)
//
template double ComputeSNR(i3d::Image3d<i3d::GRAY16> const& img,
                           i3d::Image3d<i3d::GRAY16> const& mask);

} // namespace mitogen
