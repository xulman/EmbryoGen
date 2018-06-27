#ifndef VOIDDISPLAYUNIT_H
#define VOIDDISPLAYUNIT_H

#include "DisplayUnit.h"

/**
 * This class implements drawing by not doing anything at all,
 * this is essentially a placeholder unit when no drawing output
 * is desired at all.
 *
 * Author: Vladimir Ulman, 2018
 */
class VoidDisplayUnit : public DisplayUnit
{
public:
	void DrawPoint(const int,
	               const Vector3d<float>&,
	               const float,
	               const int) override
	{ }

	void DrawLine(const int,
	              const Vector3d<float>&,
	              const Vector3d<float>&,
	              const int) override
	{ }

	void DrawVector(const int,
	                const Vector3d<float>&,
	                const Vector3d<float>&,
	                const int) override
	{ }

	void DrawTriangle(const int,
	                  const Vector3d<float>&,
	                  const Vector3d<float>&,
	                  const Vector3d<float>&,
	                  const int) override
	{ }
};
#endif
