#include "SceneryDisplayUnit.h"
#include <sstream>
#include <zmq.hpp>

void SceneryDisplayUnit::DrawPoint(const Vector3d<float>& pos,
                                   const float radius,
                                   const int color)
{
		//for now...
		REPORT(pos << ", radius=" << radius << ", color=" << color);
}


void SceneryDisplayUnit::DrawLine(const Vector3d<float>& posA,
                                  const Vector3d<float>& posB,
                                  const int color)
{
		//for now...
		REPORT(posA << " <-> " << posB << ", color=" << color);
}


void SceneryDisplayUnit::DrawVector(const Vector3d<float>& pos,
                                    const Vector3d<float>& vector,
                                    const int color)
{
	//for now...
	REPORT(vector << " @ " << pos << ", color=" << color);
}


void SceneryDisplayUnit::DrawTriangle(const Vector3d<float>& posA,
                                      const Vector3d<float>& posB,
                                      const Vector3d<float>& posC,
                                      const int color)
{
	//for now...
	REPORT(posA << ", " << posB << ", " << posC << ", color=" << color);
}


void SceneryDisplayUnit::ConnectToHost(void)
{
	context  = new zmq::context_t(1);
	socket   = new zmq::socket_t(*context, ZMQ_PAIR);
	socket->connect(std::string("tcp://")+std::string(hostUrl));
}

void SceneryDisplayUnit::DisconnectFromHost(void)
{
	if (socket != NULL)
	{
		socket->disconnect(std::string("tcp://")+std::string(hostUrl));
		socket->close();
		delete socket;
		socket = NULL;
	}

	if (context != NULL)
	{
		delete context;
		context = NULL;
	}
}
