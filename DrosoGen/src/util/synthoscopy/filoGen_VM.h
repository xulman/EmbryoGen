#ifndef _UTIL_SYNTHOSCOPY_FILOGEN_H_
#define _UTIL_SYNTHOSCOPY_FILOGEN_H_

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

namespace filogen
{

//------------------------------------------------------------------------
// simulation of microscope
//------------------------------------------------------------------------
void PhaseII(i3d::Image3d<float>& fimg,
             const i3d::Image3d<float> &psf,
             float factor = 1.0f);

//------------------------------------------------------------------------
// simulation of acquisition device
//------------------------------------------------------------------------
void PhaseIII(i3d::Image3d<float>& blurred,
              i3d::Image3d<i3d::GRAY16>& texture);

}
#endif
