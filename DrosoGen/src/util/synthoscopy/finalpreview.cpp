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

#include "../report.h"
#include "../rnd_generators.h"
#include "finalpreview.h"

#define GENERATE_WITH_HIGHSNR

template <class MV, class PV>
void PrepareFinalPreviewImage(i3d::Image3d<PV> &phantom,
										i3d::Image3d<MV> &mask,
										i3d::Image3d<PV> &final)
{
	if (phantom.GetSize() != mask.GetSize())
		throw ERROR_REPORT("Phantom and mask images do not match in size.");

	//make the final image, into which we will prepare/draw, similar to the phantom image
	//note: also allocates memory
	final.CopyMetaData(phantom);

	//should somewhat enlarge histogram spread and shift it
#ifdef GENERATE_WITH_HIGHSNR
	#define SIGNAL_SHIFT	700.0f
	#define RON_VARIANCE	90.0f
	#define PSF_MULT	1.f
#else
	#define SIGNAL_SHIFT	700.0f
	#define RON_VARIANCE	270.0f
	#define PSF_MULT	1.f
	//was: 2.2f
#endif

	//local float copy of input image
	i3d::Image3d<float> tmp;
	tmp.CopyMetaData(phantom);

	//create it
	//
	//and hack to emphasize the cell shape: this introduces
	//at least 1 molecule to be present everywhere in the cell, the phantom
	//image may increase this number anywhere in the cell in addition
	float* pF=tmp.GetFirstVoxelAddr();
	PV* pV=phantom.GetFirstVoxelAddr();
	PV* const LastpV=phantom.GetFirstVoxelAddr() + phantom.GetImageSize();
	const MV* pM=mask.GetFirstVoxelAddr();

	for (; pV != LastpV; ++pF, ++pV, ++pM)
	{
		*pF=(float)*pV; //the copy
		if (*pM)
		{
			*pF += 0.2f;
			*pF *= 600.0f;
		}
	}

	//simulate PSF:
	//PSF from "psf/2012-10-24_0_0_10_1_0_1_0_4_0_0_0_0_4_7.ics" was
	//observed to have sigma 1.6px @ 65nm/px in x,y -> 0.104um
	//observed to have sigma 4.5px @ 100nm/px in z  -> 0.45um
	i3d::GaussIIR(tmp,
		PSF_MULT*0.13f*tmp.GetResolution().GetX(),
		PSF_MULT*0.13f*tmp.GetResolution().GetY(),
		//PSF_MULT*0.15f*tmp.GetResolution().GetZ());	//2D
		PSF_MULT*0.26f*tmp.GetResolution().GetZ());	//3D
	//note: params were adjusted a little bit to just look better :(
	//
	//this typically returns values in <0,1> interval,
	//which thus can't be directly converted into integers
	

	//running pointers
	pF=tmp.GetFirstVoxelAddr();
	pV=final.GetFirstVoxelAddr();
	PV* const LastpVf=final.GetFirstVoxelAddr() + final.GetImageSize();
	pM=mask.GetFirstVoxelAddr();

	for (; pV != LastpVf; ++pF, ++pV, ++pM)
	{
		//uncertainty in the number of incoming photons
		float noiseMean = std::sqrt(*pF), // from statistics: shot noise = sqrt(signal)
				noiseVar = noiseMean;       // for Poisson distribution E(X) = D(X)

		*pV=static_cast<PV>(std::ceil(*pF + GetRandomPoisson(noiseMean) - noiseVar));

		//constants are parameters of Andor iXon camera provided from vendor:
		//photon shot noise: dark current
		*pV+=static_cast<PV>( GetRandomPoisson(0.06f) );

		//read-out noise:
		//  variance up to 25.f (old camera on ILBIT)
		//  variance about 1.f (for the new camera on ILBIT)
		*pV+=static_cast<PV>( std::max(0.f,GetRandomGauss(SIGNAL_SHIFT,RON_VARIANCE)) );
		//note: this is the dominating noise, this one will be calculated with
	}

#ifdef GENERATE_WITH_ANISOTROPIC_GT
	// resample all images to the final desired anisotropic resolution
	i3d::Resolution finalRes; //TODO define some!

	REPORT("Resampling from resolution: " << mask.GetResolution().GetRes());
	REPORT("Resampling to resolution: " << finalRes.GetRes());

	i3d::ResampleToDesiredResolution(phantom, finalRes, i3d::NEAREST_NEIGHBOUR);
	i3d::ResampleToDesiredResolution(mask,    finalRes, i3d::NEAREST_NEIGHBOUR);
	i3d::ResampleToDesiredResolution(final,   finalRes, i3d::NEAREST_NEIGHBOUR);
#endif
}


//
// explicit instantiations
//
template void PrepareFinalPreviewImage(i3d::Image3d<i3d::GRAY8>  &phantom, i3d::Image3d<bool> &mask,
													i3d::Image3d<i3d::GRAY8>  &final);
template void PrepareFinalPreviewImage(i3d::Image3d<i3d::GRAY16> &phantom, i3d::Image3d<bool> &mask,
													i3d::Image3d<i3d::GRAY16> &final);
template void PrepareFinalPreviewImage(i3d::Image3d<i3d::GRAY8>  &phantom, i3d::Image3d<i3d::GRAY8> &mask,
													i3d::Image3d<i3d::GRAY8>  &final);
template void PrepareFinalPreviewImage(i3d::Image3d<i3d::GRAY16> &phantom, i3d::Image3d<i3d::GRAY8> &mask,
													i3d::Image3d<i3d::GRAY16> &final);
template void PrepareFinalPreviewImage(i3d::Image3d<i3d::GRAY8>  &phantom, i3d::Image3d<i3d::GRAY16> &mask,
													i3d::Image3d<i3d::GRAY8>  &final);
template void PrepareFinalPreviewImage(i3d::Image3d<i3d::GRAY16> &phantom, i3d::Image3d<i3d::GRAY16> &mask,
													i3d::Image3d<i3d::GRAY16> &final);
