/*---------------------------------------------------------------------

texture.h

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

#pragma once

#include <i3d/image3d.h>

/**
 * Generate Perlin noise and put the results into image3d<float> image:
 *
 * var ... variability of the output: larger -> larger blobs
 * alpha ... smoothness:              larger -> secondary frequencies less
 * strong larger -> smoother edges beta ... flickering                larger ->
 * secondary frequencies less similar (more distant) larger -> more of the
 * "speckle noise" n ... fineness                     larger -> more of the
 * secondary frequencies
 */

//------------------------------------------------------------------------

void DoPerlin3D(i3d::Image3d<float>& fimg,
                double var,
                double alpha = 8,
                double beta = 4,
                int n = 6);

//------------------------------------------------------------------------
/*template <class VOXEL> void GenPerlin(
        i3d::Image3d<VOXEL> &img, // empty already allocated image
        float variance,
        float alpha = 8,
        float beta = 4,
        float influence = 1.0f);*/

//------------------------------------------------------------------------

template <class VOXEL>
void AddPerlin(i3d::Image3d<VOXEL>& img, // image with data
               float variance,
               float alpha = 8,
               float beta = 4,
               float influence = 1.0f);

//------------------------------------------------------------------------

template <class VOXEL>
void Stretch(i3d::Image3d<VOXEL>& img, // image with data
             VOXEL min_value,
             VOXEL max_value,
             float skewness = 1.0f);

//------------------------------------------------------------------------

void IncreaseContrast(i3d::Image3d<float>& img, float factor = 1.0f);

//------------------------------------------------------------------------

template <class VOXEL>
VOXEL ComputeQuantileIntensity(const i3d::Image3d<VOXEL>& img, float quantile);

//------------------------------------------------------------------------
//
