#pragma once

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

#include <i3d/image3d.h>

namespace mitogen {

/**
 * \ingroup toolbox
 *
 * Computes preview of final image, which is to simulate transmission
 * of the phantom image \e phantom through optical system
 * and its acquisition in the sensor device.
 *
 * \param[in,out] phantom	phantom image (the input phantom image can be
 * modified!) \param[out] final			finale preview image \param[in]
 * configIni		configuration of the simulation
 *
 * The \e phantom and \e mask images must be of the same size and resolution.
 * It is expected that both of them be isotropic, but it is technically not
 * required.
 */
template <class PV, class FV>
void PrepareFinalPreviewImage(i3d::Image3d<PV>& phantom,
                              i3d::Image3d<FV>& final);

} // namespace mitogen
