/*---------------------------------------------------------------------

myround.cc

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

Author: Martin Maska, David Svoboda

Description: Rounding routines.

-------------------------------------------------------------------------*/

#include <i3d/basic.h>
#include "myround.hpp"

/****************************************************************************/

template <class T> void RoundFloat(float value, T &result)
{
	result = static_cast<T>(value + 0.5f);
}

template <> void RoundFloat(float value, float &result)
{
	result = value;
}

template <> void RoundFloat(float value, double &result)
{
	result = static_cast<double>(value);
}

/****************************************************************************/
/** Explicit instantiations */
template void RoundFloat(float value, i3d::GRAY8 &result);
template void RoundFloat(float value, i3d::GRAY16 &result);
template void RoundFloat(float value, int &result);

/****************************************************************************/

