#ifndef SCENERYDISPLAYUNIT_H
#define SCENERYDISPLAYUNIT_H

#include "DisplayUnit.hpp"
#include <string>
//#include <zmq.hpp>

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
	SceneryDisplayUnit(const std::string& _hostUrl)
		: hostUrl(_hostUrl)
	{
		ConnectToHost();
	}

	SceneryDisplayUnit(const char* _hostUrl)
		: hostUrl(_hostUrl)
	{
		ConnectToHost();
	}

	~SceneryDisplayUnit()
	{
		DisconnectFromHost();
	}


	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) override;

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override;

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override;

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override;

	void Tick(const char* msg) override;

protected:
	/** URL of the host (a Scenery renderer) to which we are connected */
	std::string hostUrl;

	//zmq::context_t* context = NULL;
	//zmq::socket_t*  socket  = NULL;

	void ConnectToHost(void);
	void DisconnectFromHost(void);
};
#endif
