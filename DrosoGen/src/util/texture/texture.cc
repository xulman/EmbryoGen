/*---------------------------------------------------------------------

texture.cc

This file is part of CytoPacq

Copyright (C) 2007-2013 -- Centre for Biomedical Image Analysis (CBIA)

CytoPacq is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CytoPacq is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CytoPacq. If not, see <http://www.gnu.org/licenses/>.

Author: David Svoboda, Martin Maska

Description: Manipulation with procedural texture (Perlin noise).

-------------------------------------------------------------------------*/

#ifdef _MSC_VER
	#include <time.h>
#endif

#include <i3d/histogram.h>
#include <i3d/transform.h>

#include "perlin.h"
#include "myrandom.h"
#include "texture.h"
#include "myround.h"
#include "sim_macros.h"

using namespace i3d;
using namespace std;

/***************************************************************************/
/** Generate 3D Perlin noise and store the result **/
void DoPerlin3D(Image3d<float> &fimg,
						double var,
						double alpha,
						double beta,
						int n)
{
	 if (!fimg.GetResolution().IsDefined())
	 {
		  throw ERROR_DETAILS("DoPerlin3D: Image resolution is not set.");
	 }

	 // 'var' is in microns - function PerlinNoise3D requires pixels as
	 // the reference unit. We must convert it.
	 Vector3d<float> res = fimg.GetResolution().GetRes();
	 double v_x = var * res.x,
			  v_y = var * res.y,
			  v_z = var * res.z;
	 
	 // First, place the image in the random position within the space.
	 // This guarantee that the result won't be the same in all the cases!
 
	 float shift_x = 1000*(static_cast<float>(rand())/RAND_MAX);
	 float shift_y = 1000*(static_cast<float>(rand())/RAND_MAX);
	 float shift_z = 1000*(static_cast<float>(rand())/RAND_MAX);

	 for (size_t i=0; i<fimg.GetImageSize(); i++)
			 {
				 double x,y,z;

				 x = shift_x + fimg.GetX(i);
				 y = shift_y + fimg.GetY(i);
				 z = shift_z + fimg.GetZ(i);
				 
				 float noise = (float) PerlinNoise3D(x/v_x, y/v_y, z/v_z, alpha, beta, n);

				 fimg.SetVoxel(i, noise);
			  }
}

/***************************************************************************/

template <class VOXEL> void AddPerlin(
		Image3d<VOXEL> &img,
		float variance, 
		float alpha,
		float beta,
		float skew
		)
{
#ifdef DEBUG
	echo("perlin (" << variance << "," << alpha << "," <<
		  beta << "," << skew << ")");
#endif
		
	  Image3d<float> fimg;
	  fimg.CopyMetaData(img);
	  DoPerlin3D(fimg, variance, alpha, beta);

	  float mmin = fimg.GetVoxelData().min();
	  float mmax = fimg.GetVoxelData().max();

	  for (size_t i=0; i<img.GetImageSize(); i++)
	  {
		  // put the result of Perlin noise in the place where the object
		  // appears only
		  VOXEL value = img.GetVoxel(i);

		  if (value > 0)
		  {
			  float noise = fimg.GetVoxel(i);

		  	  // normalize the noise value
		     float value_0_to_1 = (noise-mmin)/(mmax-mmin);
			  // remap to the range <-1;1>
			  float value_minus1_to_plus1 = 2 * value_0_to_1 - 1.0f;
			  // power the value, forget the sign and compute the n-th root.
			  float value_powered = pow(abs(value_minus1_to_plus1), 1.0f/skew);
			  // keep the sign
			  int sign = (value_minus1_to_plus1 > 0) ? 1 : -1;
			  float value_powered_with_sign = sign * value_powered;
			  // remap back to the range <0;1>
			  float new_val = (value_powered_with_sign + 1.0f) / 2.0f;

			  new_val *= static_cast<float>(value);
			  new_val = max(new_val, (float) std::numeric_limits<VOXEL>::min());
			  new_val = min(new_val, (float) std::numeric_limits<VOXEL>::max());

			  img.SetVoxel(i, (VOXEL) new_val);
		  }
			  
	  }
}

/***************************************************************************/

template <class VOXEL> void Stretch(i3d::Image3d<VOXEL> &img,
									VOXEL min_value,
									VOXEL max_value,
									float skewness)
{
	if (max_value < min_value)
	{
		throw ERROR_DETAILS("Incorrect stretching range!");
	}

	size_t num = img.GetImageSize();

	VOXEL mmin = std::numeric_limits<VOXEL>::max();
	VOXEL mmax = std::numeric_limits<VOXEL>::min();

	VOXEL *img_ptr = img.GetFirstVoxelAddr();
	
	// find minimal and maximal intensities in the image
	for (size_t i = 0; i < num; i++)
	{
		mmin = std::min(mmin, *img_ptr);
		mmax = std::max(mmax, *img_ptr);
		img_ptr++;
	}

	img_ptr = img.GetFirstVoxelAddr();
	float idiff = float(mmax - mmin);
	float rdiff = float(max_value - min_value);
	float value;

	// stretch the image intensities
	for (size_t i = 0; i< num; i++)
	{
		value = powf(float(*img_ptr - mmin) / idiff, skewness);
		value = min_value + value * rdiff;
		RoundFloat(value, *img_ptr);
		img_ptr++;
	 }
}

/***************************************************************************/

void IncreaseContrast(i3d::Image3d<float> &img, float factor)
{
	 float imin = img.GetVoxelData().min();
	 float imax = img.GetVoxelData().max();

	 Stretch(img, 0.0f, 1.0f);

	 for (size_t i=0; i<img.GetImageSize(); i++)
	 {
		 float value = img.GetVoxel(i);

		 if (value < 0.5f)
			  value = powf(2.0f * value, factor) / 2.0f;
		 else
			  value = powf(2.0f * (value - 0.5f), 1.0f / factor) / 2.0f + 0.5f;

		 img.SetVoxel(i, value);
	 }

	 Stretch(img, imin, imax);
}


/***************************************************************************/

template <class VOXEL> VOXEL ComputeQuantileIntensity(const i3d::Image3d<VOXEL> &img, float quantile)
{
	i3d::Histogram hist;
	i3d::IntensityHist(img, hist);

	unsigned long numberOfQuantilePixels = static_cast<unsigned long>((quantile / 100.0f) * float(img.GetImageSize()));
	size_t histBinIndex = 0;
	unsigned long tempSum = 0;	
		
    while (numberOfQuantilePixels > tempSum)
	{
		tempSum += hist[histBinIndex];
		histBinIndex++;
	}
	
    float intensity = float(histBinIndex - 1) + (float(tempSum - numberOfQuantilePixels) / float(hist[histBinIndex - 1]));

	VOXEL result;
	RoundFloat(intensity, result);

	return result;
}

/***************************************************************************/
/* explicit instantiations */
template void AddPerlin(Image3d<GRAY8> &img,
								float variance, 
								float alpha,
								float beta,
								float);

template void AddPerlin(Image3d<GRAY16> &img,
								float variance, 
								float alpha,
								float beta,
								float);

template void AddPerlin(Image3d<float> &img,
								float variance, 
								float alpha,
								float beta,
								float);

template void Stretch(i3d::Image3d<i3d::GRAY8> &img, 
							 i3d::GRAY8 min_value, 
							 i3d::GRAY8 max_value, 
							 float skewness);

template void Stretch(i3d::Image3d<i3d::GRAY16> &img, 
							 i3d::GRAY16 min_value, 
							 i3d::GRAY16 max_value, 
							 float skewness);

template void Stretch(i3d::Image3d<float> &img, 
							 float min_value, 
							 float max_value, 
							 float skewness);

template 
i3d::GRAY8 ComputeQuantileIntensity(const i3d::Image3d<i3d::GRAY8> &img, 
												float quantile);

template 
i3d::GRAY16 ComputeQuantileIntensity(const i3d::Image3d<i3d::GRAY16> &img, 
												 float quantile);

template 
float ComputeQuantileIntensity(const i3d::Image3d<float> &img, 
										 float quantile);

/***************************************************************************/
