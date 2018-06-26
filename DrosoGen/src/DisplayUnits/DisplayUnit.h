#ifndef DISPLAYUNIT_H
#define DISPLAYUNIT_H

#include "../Vector3d.h"

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
	    exist at the drawing side. Every point may be recognized
	    with its fixed ID, so that the displaying side can update it. */
	virtual
	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) =0;

	/** Draws a line between two points 'posA' and 'posB'.
	    'color' is an index to a color look up table that might
	    exist at the drawing side. Every line may be recognized
	    with its fixed ID, so that the displaying side can update it. */
	virtual
	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) =0;

	/** Draws a 'vector' often as an arrow, positioned at 'pos'.
	    'color' is an index to a color look up table that might
	    exist at the drawing side. Every vector may be recognized
	    with its fixed ID, so that the displaying side can update it. */
	virtual
	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) =0;

	/** Draws a triangle between the three points. The order of them
	    matters as the normal is calculated as 'posB-posA' x 'posC-posA'.
	    'color' is an index to a color look up table that might
	    exist at the drawing side. Every triangle may be recognized
	    with its fixed ID, so that the displaying side can update it. */
	virtual
	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) =0;

	/** Forces the unit to display/send all what it has possibly buffered
	    immediately. Some units may, typically for performance reasons, buffer
	    all drawing request (see the functions above) and this method is here
	    to notify them to submit/process the buffer (and empty it consequently).
	    Many units, however, process the drawing requests immediately and this
	    method is kept empty for them. */
	virtual
	void Flush(void) {};
};
#endif
