/**********************************************************************
*
* finalpreview.cpp
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
* Description:
* A collection of functions simulating the aberrations and imperfections
* caused by an optical system (microscope), and sampling and noise generation
* cause by an acquisition device (CCD camera).
*
***********************************************************************/

#include <i3d/image3d.h>
#include <i3d/filters.h>
#include <i3d/morphology.h>
#include <i3d/transform.h>

#include "../report.hpp"
#include "../rnd_generators.hpp"
#include "finalpreview.hpp"

namespace mitogen
{

template <class PV, class FV>
void PrepareFinalPreviewImage(i3d::Image3d<PV> &phantom,
										i3d::Image3d<FV> &final)
{
	i3d::GaussIIR(phantom,
		0.13f*phantom.GetResolution().GetX(),
		0.13f*phantom.GetResolution().GetY(),
		//0.15f*phantom.GetResolution().GetZ());	//2D
		0.26f*phantom.GetResolution().GetZ());	//3D
	
	//running pointers
	PV* pF = phantom.GetFirstVoxelAddr();
	PV* const pFlast = pF + phantom.GetImageSize();
	for (; pF != pFlast; ++pF)
	{
		//uncertainty in the number of incoming photons
		const float noiseMean = std::sqrt(*pF); // from statistics: shot noise = sqrt(signal)
		*pF += (PV)GetRandomPoisson(noiseMean) - noiseMean;

		//constants are parameters of Andor iXon camera provided from vendor:
		//photon shot noise: dark current
		*pF += (PV)GetRandomPoisson(0.06f);

		//read-out noise:
		//  variance up to 25.f (old camera on ILBIT)
		//  variance about 1.f (for the new camera on ILBIT)
		*pF += std::max(0.f,GetRandomGauss(700.f,90.f));
	}

	i3d::FloatToGrayNoWeight(phantom,final);

#ifdef GENERATE_WITH_ANISOTROPIC_GT
	// resample all images to the final desired anisotropic resolution
	i3d::Resolution finalRes; //TODO define some!

	//REPORT("Resampling from resolution: " << mask.GetResolution().GetRes());
report::message(fmt::format("Resampling to resolution: {}" , finalRes.GetRes()));

	i3d::ResampleToDesiredResolution(phantom, finalRes, i3d::NEAREST_NEIGHBOUR);
	//i3d::ResampleToDesiredResolution(mask,    finalRes, i3d::NEAREST_NEIGHBOUR);
	i3d::ResampleToDesiredResolution(final,   finalRes, i3d::NEAREST_NEIGHBOUR);
#endif
}


//
// explicit instantiations (just one, for now...)
//
template void PrepareFinalPreviewImage(i3d::Image3d<float> &phantom,
                                       i3d::Image3d<i3d::GRAY16> &final);

} //end of the namespace
