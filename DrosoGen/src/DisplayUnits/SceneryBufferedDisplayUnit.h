#ifndef SCENERYBUFFEREDDISPLAYUNIT_H
#define SCENERYBUFFEREDDISPLAYUNIT_H

#include "SceneryDisplayUnit.h"

/**
 * This class implements drawing by sending the drawing commands
 * to a SimView (Scenery/SciView Java server) that draws it within
 * its 3D interactive window. The commands are sent over a network,
 * so the server need not live on the same computer.
 *
 * The commands are however buffered and sent only when Flush() is called.
 *
 * Author: Vladimir Ulman, 2018
 */
class SceneryBufferedDisplayUnit : public SceneryDisplayUnit
{
public:
	SceneryBufferedDisplayUnit(const std::string& _hostUrl) :
		SceneryDisplayUnit(_hostUrl)
	{ }
	//NB: relying on the fact that Buffered* are initiated below

	SceneryBufferedDisplayUnit(const char* _hostUrl) :
		SceneryDisplayUnit(_hostUrl)
	{ }

	//destructor is used from the parent

	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0);

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0);

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0);

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0);

	void Flush(void);

	void InitBuffers(void)
	{
		BufferedPointMsgs.str("");
		BufferedPointMsgs_count = 0;

		BufferedLineMsgs.str("");
		BufferedLineMsgs_count = 0;

		BufferedVectorMsgs.str("");
		BufferedVectorMsgs_count = 0;

		BufferedTriangleMsgs.str("");
		BufferedTriangleMsgs_count = 0;
	}

private:
	//buffers for the four drawing primitives...
	std::ostringstream BufferedPointMsgs;
	std::ostringstream BufferedLineMsgs;
	std::ostringstream BufferedVectorMsgs;
	std::ostringstream BufferedTriangleMsgs;

	//...and the count of points stored in them
	int BufferedPointMsgs_count;
	int BufferedLineMsgs_count;
	int BufferedVectorMsgs_count;
	int BufferedTriangleMsgs_count;
};
#endif
