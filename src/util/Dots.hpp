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

#pragma once

#include "Vector3d.hpp"

/**
 * The basic minimalistic class describing one particular dot.
 */
class Dot {
  public:
	/** empty/default constructor: inits especially positions to (0,0,0) */
	Dot() : pos(0.f, 0.f, 0.f), cntOfExcitations(0), refractiveIdx(1){};

	/** "add new dot" constructor: passes its arguments into the class
	 * attributes */
	Dot(Vector3d<float> const& POS,
	    const short CNTOFEXCITATIONS = 0,
	    const float REFRACTIVE = 1)
	    : pos(POS), cntOfExcitations(CNTOFEXCITATIONS),
	      refractiveIdx(REFRACTIVE){};

	/** standard "copy" constructor; use the constructor above to add daughter
	 * dots */
	Dot(Dot const& DOT) {
		pos = DOT.pos;
		cntOfExcitations = DOT.cntOfExcitations;
		refractiveIdx = DOT.refractiveIdx;
	}

	/** destructor: does nothing, currently */
	~Dot(){};

	/** the current 3D coordinate of the dot, in microns */
	Coord3d<float> pos;

	/** assuming dot rendering simulates exposition of the dots to the
	   excitation light for always the same time period, it is enough to count
	   how many times this has happened in order to be able to calculate actual
	   (decreasing -- due to the photobleaching) photon budget; this counter is
	   increased with every rendering of this dot into some phantom image */
	short cntOfExcitations = 0;

	/** refractive index of this dot */
	float refractiveIdx = 1;
};
