#include "SceneryDisplayUnit.hpp"
#include "../util/report.hpp"
#include <sstream>
//#include <zmq.hpp>

void SceneryDisplayUnit::DrawPoint(const int ID,
                                   const Vector3d<float>& pos,
                                   const float radius,
                                   const int color)
{
	std::ostringstream msg;

	//message syntax := v1 points N dim D Point1 ... PointN
	//  Point syntax := ID x1 ... xD radius color
	//
	//where N is the number of points in this message,
	//where D is the dimension of one point,
	//where x1,...,xD,radius are real scalars, ID,color are integer scalars

	msg << "v1 points 1 dim 3 " << ID << " "
	    << pos.x << " " << pos.y << " " << pos.z << " "
	    << radius << " " << color;

	std::string msgString(msg.str());
	//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
}


void SceneryDisplayUnit::DrawLine(const int ID,
                                  const Vector3d<float>& posA,
                                  const Vector3d<float>& posB,
                                  const int color)
{
	std::ostringstream msg;

	//  message syntax := v1 lines N dim D PointPair1 ... PointPairN
	//PointPair syntax := ID o1 ... oD p1 ... pD color
	//
	//where N is the number of lines in this message,
	//where D is the dimension of one line end point,
	//where o1,...,oD,p1,...,pD are real scalars, ID,color are integer scalars
	//
	// o,p are two end points of a line

	msg << "v1 lines 1 dim 3 " << ID << " "
	    << posA.x << " " << posA.y << " " << posA.z << " "
	    << posB.x << " " << posB.y << " " << posB.z << " "
	    << color;

	std::string msgString(msg.str());
	//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
}


void SceneryDisplayUnit::DrawVector(const int ID,
                                    const Vector3d<float>& pos,
                                    const Vector3d<float>& vector,
                                    const int color)
{
	std::ostringstream msg;

	//  message syntax := v1 vectors N dim D PointPair1 ... PointPairN
	//PointPair syntax := ID o1 ... oD p1 ... pD color
	//
	//where N is the number of vectors in this message,
	//where D is the dimension of the base coordinate and vector itself
	//where o1,...,oD,p1,...,pD are real scalars, ID,color are integer scalars
	//
	// o is the base coordinate/anchor point of the vector, p is the vector itself
	//
	//vector  ID   essentially determines to which cell it belongs
	//vector color essentially determines what type this vector is

	msg << "v1 vectors 1 dim 3 " << ID << " "
	    << pos.x    << " " << pos.y    << " " << pos.z    << " "
	    << vector.x << " " << vector.y << " " << vector.z << " "
	    << color;

	std::string msgString(msg.str());
	//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
}


void SceneryDisplayUnit::DrawTriangle(const int ID,
                                      const Vector3d<float>& posA,
                                      const Vector3d<float>& posB,
                                      const Vector3d<float>& posC,
                                      const int color)
{
	std::ostringstream msg;

	//     message syntax := v1 triangles N dim D PointTripple1 ... PointTrippleN
	//PointTripple syntax := ID o1 ... oD p1 ... pD q1 ... qD color
	//
	//where N is the number of triangles in this message,
	//where D is the dimension of one triangle vertex,
	//where o1,...,oD,p1,...,pD,q1,...,qD are real scalars,
	//where ID,color are integer scalars
	//
	// o,p,q are three vertices of a triangle

	msg << "v1 triangles 1 dim 3 " << ID << " "
	    << posA.x << " " << posA.y << " " << posA.z << " "
	    << posB.x << " " << posB.y << " " << posB.z << " "
	    << posC.x << " " << posC.y << " " << posC.z << " "
	    << color;

	std::string msgString(msg.str());
	//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
}


void SceneryDisplayUnit::Tick(const std::string& msg)
{
	std::string msgString = std::string("v1 tick ") + msg;
	//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
}


void SceneryDisplayUnit::ConnectToHost(void)
{
	/*
	context  = new zmq::context_t(1);
	socket   = new zmq::socket_t(*context, ZMQ_PAIR);
report::debugMessage(fmt::format("Connecting Scenery DU to: {}" , (std::string("tcp://")+std::string(hostUrl))));
	socket->connect(std::string("tcp://")+std::string(hostUrl));
	*/
}

void SceneryDisplayUnit::DisconnectFromHost(void)
{
	/*
	if (socket != NULL)
	{
report::debugMessage(fmt::format("Disconnecting Scenery DU from: {}" , (std::string("tcp://")+std::string(hostUrl))));
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
	*/
}
