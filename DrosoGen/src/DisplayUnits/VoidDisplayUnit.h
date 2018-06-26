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
	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) override
	{ }

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override
	{ }

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override
	{ }

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override
	{ }
};
#endif
