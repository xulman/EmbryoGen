#ifndef DISPLAYUNIT_H
#define DISPLAYUNIT_H

#include "../params.h"

/**
 * This class is essentially only a collection of (empty) functions
 * that any drawing/displaying unit should implement.
 *
 * Author: Vladimir Ulman, 2018
 */
class DisplayUnit
{
public:
	/** Draws a point at 'pos', often as a sphere of 'radius'.
	    'color' is an index to a color look up table that might
		 exist at the drawing side. */
	virtual
	void DrawPoint(const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) =0;

	/** Draws a line between two points 'posA' and 'posB'.
	    'color' is an index to a color look up table that might
		 exist at the drawing side. */
	virtual
	void DrawLine(const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) =0;

	/** Draws a 'vector' often as an arrow, positioned at 'pos'.
	    'color' is an index to a color look up table that might
		 exist at the drawing side. */
	virtual
	void DrawVector(const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) =0;

	/** Draws a triangle between the three points. The order of them
	    matters as the normal is calculated as 'posB-posA' x 'posC-posA'.
	    'color' is an index to a color look up table that might
		 exist at the drawing side. */
	virtual
	void DrawTriangle(const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) =0;
};
#endif