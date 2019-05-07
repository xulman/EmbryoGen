/**********************************************************************
*
* dots.h
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
* Description: Definition of class 'Dot' that describes
* the point-like signals in the specimen.
*
***********************************************************************
*
* This file is actually modified from the original and the Dot class
* is stripped down largely in terms of the memory footprint per one dot.
*
* In FiloGen's terminology, the Dot is equivalent to a fluorescent particle.
*
***********************************************************************/

#ifndef MITOGEN_DOTS_H
#define MITOGEN_DOTS_H

#include "Vector3d.h"

/**
 * The basic minimalistic class describing one particular dot.
 */
class Dot
{
public:
	/** empty/default constructor: inits especially positions to (0,0,0) */
	Dot():
		pos(0.f,0.f,0.f), birth(0), refractiveIdx(0) {};

	/** "add new dot" constructor: passes its arguments into the class attributes */
	Dot(Vector3d<float> const & POS, const short BIRTH=0, const float REFRACTIVE=0):
		pos(POS), birth(BIRTH), refractiveIdx(REFRACTIVE) {};

	/** standard "copy" constructor; use the constructor above to add daughter dots */
	Dot(Dot const & DOT) {
		pos           = DOT.pos;
		birth         = DOT.birth;
		refractiveIdx = DOT.refractiveIdx;
	}

	/** destructor: does nothing, currently */
	~Dot() {};

	/** the current 3D coordinate of the dot, in microns */
	Coord3d<float> pos;

	/** time point in units of frames (not real time) when this dot started to exist,
	    this is mainly used for the simulation of the photobleaching (as an alternative
	    to storing a remaining dot's photon capacity to emit */
	short birth;

	/** refractive index of this dot */
	float refractiveIdx = 0;
};
#endif
