/*
 * Virtual Microscope - simulation engine
 *
 * Copyright (C) 2000-2018   Centre for Biomedical Image Analysis (CBIA)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * FILE: main.cc
 *
 * Functions for imitation of optical microscope and CCD camera
 *
 * David Svoboda <svoboda@fi.muni.cz> 2018
 * Martin Maska <xmaskaa@fi.muni.cz> 2018
 */


///------------------------------------------------------------------------

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <i3d/image3d.h>
#include <i3d/transform.h>
#include <i3d/filters.h>
#include <i3d/convolution.h>
#include "../rnd_generators.h"
#include "../texture/texture.h"
#include "../report.h"
#include "Filogen_VM.h"

///------------------------------------------------------------------------
namespace filogen
{

#define SQR(x) ((x)*(x))

///------------------------------------------------------------------------

void PhaseII(i3d::Image3d<float>& fimg,
             const i3d::Image3d<float> &psf,
             float factor)
{
	// note: the phantom image is already of the float
	// data type (to enable convolution with PSF)

	// perform intensity scaling of the phantom by the given factor
	for (size_t i = 0; i < fimg.GetImageSize(); ++i)
	{
		fimg.SetVoxel(i, fimg.GetVoxel(i) * factor);
	}

	// If the PSF has different resolution from the processed image, 
	// resampling is required.
	i3d::Vector3d<float> 
			  res_psf = psf.GetResolution().GetRes(),
			  res_fimg = fimg.GetResolution().GetRes();

	DEBUG_REPORT("res img: " << res_fimg);
	DEBUG_REPORT("res psf: " << res_psf);

	i3d::Image3d<float> psfII(psf);

	if (res_fimg != res_psf)
	{
		DEBUG_REPORT("resampling psf");
		i3d::ResampleToDesiredResolution(psfII, res_fimg, i3d::LANCZOS);
	}

	DEBUG_REPORT("image size: " << fimg.GetSize());
	DEBUG_REPORT("psf size: " << psfII.GetSize());

	// convolution with real confocal PSF
	i3d::Image3d<float> blurred_texture;
	i3d::Convolution<double,float,float>(fimg, psfII, blurred_texture);

	DEBUG_REPORT("convolution done.");

	// Let us add the uneven illumination (with its brightest loci in the cell
	// centre, which happens to be in the image centre)
	const int xC=(int)(blurred_texture.GetSizeX()/2);
	const int yC=(int)(blurred_texture.GetSizeY()/2);
	// max distance
	const float maxDist = SQR((float)xC) + SQR((float)yC);

	float* f=fimg.GetFirstVoxelAddr();
	float* p=blurred_texture.GetFirstVoxelAddr();
	for (int z=0; z < (signed)blurred_texture.GetSizeZ(); ++z)
		 for (int y=0; y < (signed)blurred_texture.GetSizeY(); ++y)
			  for (int x=0; x < (signed)blurred_texture.GetSizeX(); ++x, ++f, ++p)
			  {
				// background signal (inverted parabola)
				float distSq = 
						  -(SQR((float)(x-xC)) + SQR((float)(y-yC)))/maxDist + 1.f;
				*f = *p * distSq;
			  }

	//fimg = blurred_texture;
	DEBUG_REPORT("all done.");
}

///------------------------------------------------------------------------

const int reseedPeriod = 4000000;
rndGeneratorHandle rngExcessNoiseFactor(reseedPeriod);
rndGeneratorHandle rngPhotonNoise(reseedPeriod);
rndGeneratorHandle rngDarkCurrent(reseedPeriod);
rndGeneratorHandle rngReadoutNoise(reseedPeriod);
rndGeneratorHandle rngSKIP(reseedPeriod);

void PhaseIII(i3d::Image3d<float>& blurred,
              i3d::Image3d<i3d::GRAY16>& texture)
{
	// NONSPECIFIC BACKGROUND
	i3d::Image3d<float> bgImg;
	bgImg.CopyMetaData(blurred);
	DoPerlin3D(bgImg,10.0,7.0,1.0,10); // very smooth a wide coherent noise

	DEBUG_REPORT("BG Perlin done.");
	DEBUG_REPORT("Image contains " << blurred.GetImageSize() << " voxels");

	// reset the maximum and minimum intensity levels of the background
	// to the expected values
	const float oldMaxI = bgImg.GetVoxelData().max();
	const float oldMinI = bgImg.GetVoxelData().min();
	const float newMaxI = 2.2f;
	const float newMinI = 2.0f;
	const float scale = (newMaxI-newMinI)/(oldMaxI-oldMinI);

	for (size_t i=0; i<bgImg.GetImageSize(); i++)
	{
		float value = bgImg.GetVoxel(i);
		value = scale*(value - oldMinI) + newMinI;
	   bgImg.SetVoxel(i, value);
	}
	
   // given gain of EMCCD camera
   const int EMCCDgain = 1000;

	float* p=blurred.GetFirstVoxelAddr();
	for (size_t i=0; i<blurred.GetImageSize(); i++, p++)
	{
		// shift the signal (simulates non-ideal black background)
		// ?reflection of medium?
		*p += bgImg.GetVoxel(i);

		// ENF ... Excess Noise Factor (stochasticity of EMCCD gain)
		// source of information:
		// https://www.qimaging.com/resources/pdfs/emccd_technote.pdf
		const float ENF = GetRandomUniform(1.0f, 1.4f, rngExcessNoiseFactor);

		// PHOTON NOISE 
		// uncertainty in the number of incoming photons,
		// from statistics: shot noise mean = sqrt(signal) 
		const float noiseMean = sqrtf(*p);
		*p += ENF * ((float)GetRandomPoisson(noiseMean, rngPhotonNoise) - noiseMean);

		// avoid strong discretization of intensity histogram due
		// to the Poisson probability function (aka SKIP)
		*p += GetRandomUniform(0.0, 1.0, rngSKIP);

		// EMCCD GAIN
       // amplification of signal (and inevitably also the noise)
	   *p *= EMCCDgain; 

		// DARK CURRENT
		// constants are parameters of Andor iXon camera provided from vendor:
		*p += ENF*EMCCDgain*(float)GetRandomPoisson(0.06f, rngDarkCurrent);

		// BASELINE (camera electron level)
		*p += 400.0f;

		// READOUT NOISE
		// variance up to 25.f (old camera on ILBIT)
		// variance about 1.f (for the new camera on ILBIT)
		*p += GetRandomGauss(0.0f,1.0f, rngReadoutNoise);

		// ADC (analogue-digital converter)
		// ADCgain ... how many electrons correspond one intensity level
		// ADCoffset ... how many intensity levels are globally added to each pixel
		const float ADCgain = 28.0f; 
		*p /= ADCgain;

		const float ADCoffset = 400.0f;
		*p += ADCoffset;
	}

	//obtain final GRAY16 image
	i3d::FloatToGrayNoWeight(blurred,texture);
	DEBUG_REPORT("all done.");
}

///------------------------------------------------------------------------
} //end of the namespace
