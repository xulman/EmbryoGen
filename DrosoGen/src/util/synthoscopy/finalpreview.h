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
* Description:
* A collection of functions simulating the aberrations and imperfections
* caused by an optical system (microscope), and sampling and noise generation
* cause by an acquisition device (CCD camera).
*
***********************************************************************/

#ifndef UTIL_SYNTHOSCOPY_FINALPREVIEW_H
#define UTIL_SYNTHOSCOPY_FINALPREVIEW_H

#include <i3d/image3d.h>
#include <ini/iniparser.h>

/**
 * \ingroup toolbox
 *
 * Just computes preview of final image, which is to simulate
 * transmission of the phantom image \e phantom through optical system
 * and its acquisition in the sensor device.
 *
 * The level of noise is a bit different in the region of \e mask
 * in order to outline cell shape. The texture stored in the
 * phantom image \e phantom often does not spread over the whole \e mask
 * giving false impression of what is the real size of the cell.
 *
 * If the GTGEN_WITH_ANISOTROPIC_GT is defined, both \e phantom and
 * \e mask images are resampled to meet the resolution supplied in the
 * \e configIni file. The \e final image always meets this resolution
 * regardless of the GTGEN_WITH_ANISOTROPIC_GT state. If the macro is
 * not defined, both \e phantom and \e mask images not resampled.
 *
 * If the GTGEN_WITH_CHRCENTRESINPHANTHOMS is defined, the chromocentres
 * are removed from the phantom image (before it is turned into a final
 * image, ofcourse). So the \e phantom image is changed in this particular case.
 *
 * \param[in,out] phantom	phantom image
 * \param[in,out] mask		cell mask
 * \param[out] final			finale preview image
 * \param[in] configIni		configuration of the simulation
 *
 * The \e phantom and \e mask images must be of the same size and resolution.
 * It is expected that both of them be isotropic, but it is not required.
 *
 * The \e configIni is a reference to parameters for the Stage II and III
 * simulation routines.
 */
template <class MV, class PV>
void PrepareFinalPreviewImage(i3d::Image3d<PV> &phantom,
										i3d::Image3d<MV> &mask,
										i3d::Image3d<PV> &final,
										const IniHandler configIni);
#endif
