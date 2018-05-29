#ifndef SCENERYDISPLAYUNIT_H
#define SCENERYDISPLAYUNIT_H

#include "DisplayUnit.h"
#include "../params.h"
#include <string>

/**
 * This class implements drawing by sending the drawing commands
 * to a SimView (Scenery/SciView Java server) that draws it within
 * its 3D interactive window. The commands are sent over a network,
 * so the server need not live on the same computer.
 *
 * Author: Vladimir Ulman, 2018
 */
class SceneryDisplayUnit : public DisplayUnit
{
public:
	SceneryDisplayUnit(const std::string& _hostUrl) :
		hostUrl(_hostUrl) {}

	SceneryDisplayUnit(const char* _hostUrl) :
		hostUrl(_hostUrl) {}


	void DrawPoint(const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0);

	void DrawLine(const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0);

	void DrawVector(const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0);

	void DrawTriangle(const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0);

private:
	/** URL of the host (a Scenery renderer) to which we are connected */
	std::string hostUrl;
};
#endif
