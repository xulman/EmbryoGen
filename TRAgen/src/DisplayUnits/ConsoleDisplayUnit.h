#ifndef CONSOLEDISPLAYUNIT_H
#define CONSOLEDISPLAYUNIT_H

#include "DisplayUnit.h"
#include "../params.h"

/**
 * This class implements drawing by reporting what is being drawed
 * on the console, that is, in a textual form.
 *
 * Author: Vladimir Ulman, 2018
 */
class ConsoleDisplayUnit : public DisplayUnit
{
public:
	void DrawPoint(const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0)
	{
		REPORT(pos << ", radius=" << radius << ", color=" << color);
	}

	void DrawLine(const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0)
	{
		REPORT(posA << " <-> " << posB << ", color=" << color);
	}

	void DrawVector(const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0)
	{
		REPORT(vector << " @ " << pos << ", color=" << color);
	}

	void DrawTriangle(const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0)
	{
		REPORT(posA << ", " << posB << ", " << posC << ", color=" << color);
	}
};
#endif
