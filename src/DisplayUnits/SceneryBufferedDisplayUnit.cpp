#include "SceneryBufferedDisplayUnit.h"
#include <string>
//#include <zmq.hpp>

void SceneryBufferedDisplayUnit::DrawPoint(const int ID,
                                   const Vector3d<float>& pos,
                                   const float radius,
                                   const int color)
{
	//message syntax := v1 points N dim D Point1 ... PointN
	//  Point syntax := ID x1 ... xD radius color
	//
	//where N is the number of points in this message,
	//where D is the dimension of one point,
	//where x1,...,xD,radius are real scalars, ID,color are integer scalars

	BufferedPointMsgs << " " << ID << " "
	    << pos.x << " " << pos.y << " " << pos.z << " "
	    << radius << " " << color;
	++BufferedPointMsgs_count;
}


void SceneryBufferedDisplayUnit::DrawLine(const int ID,
                                  const Vector3d<float>& posA,
                                  const Vector3d<float>& posB,
                                  const int color)
{
	//  message syntax := v1 lines N dim D PointPair1 ... PointPairN
	//PointPair syntax := ID o1 ... oD p1 ... pD color
	//
	//where N is the number of lines in this message,
	//where D is the dimension of one line end point,
	//where o1,...,oD,p1,...,pD are real scalars, ID,color are integer scalars
	//
	// o,p are two end points of a line

	BufferedLineMsgs << " " << ID << " "
	    << posA.x << " " << posA.y << " " << posA.z << " "
	    << posB.x << " " << posB.y << " " << posB.z << " "
	    << color;
	++BufferedLineMsgs_count;
}


void SceneryBufferedDisplayUnit::DrawVector(const int ID,
                                    const Vector3d<float>& pos,
                                    const Vector3d<float>& vector,
                                    const int color)
{
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

	BufferedVectorMsgs << " " << ID << " "
	    << pos.x    << " " << pos.y    << " " << pos.z    << " "
	    << vector.x << " " << vector.y << " " << vector.z << " "
	    << color;
	++BufferedVectorMsgs_count;
}


void SceneryBufferedDisplayUnit::DrawTriangle(const int ID,
                                      const Vector3d<float>& posA,
                                      const Vector3d<float>& posB,
                                      const Vector3d<float>& posC,
                                      const int color)
{
	//     message syntax := v1 triangles N dim D PointTripple1 ... PointTrippleN
	//PointTripple syntax := ID o1 ... oD p1 ... pD q1 ... qD color
	//
	//where N is the number of triangles in this message,
	//where D is the dimension of one triangle vertex,
	//where o1,...,oD,p1,...,pD,q1,...,qD are real scalars,
	//where ID,color are integer scalars
	//
	// o,p,q are three vertices of a triangle

	BufferedTriangleMsgs << " " << ID << " "
	    << posA.x << " " << posA.y << " " << posA.z << " "
	    << posB.x << " " << posB.y << " " << posB.z << " "
	    << posC.x << " " << posC.y << " " << posC.z << " "
	    << color;
	++BufferedTriangleMsgs_count;
}


void SceneryBufferedDisplayUnit::Flush(void)
{
	if (BufferedPointMsgs_count > 0)
	{
		std::string msgString("v1 points ");
		msgString += std::to_string(BufferedPointMsgs_count);
		msgString += " dim 3";
		msgString += BufferedPointMsgs.str();

		//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
	}

	if (BufferedLineMsgs_count > 0)
	{
		std::string msgString("v1 lines ");
		msgString += std::to_string(BufferedLineMsgs_count);
		msgString += " dim 3";
		msgString += BufferedLineMsgs.str();

		//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
	}

	if (BufferedVectorMsgs_count > 0)
	{
		std::string msgString("v1 vectors ");
		msgString += std::to_string(BufferedVectorMsgs_count);
		msgString += " dim 3";
		msgString += BufferedVectorMsgs.str();

		//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
	}

	if (BufferedTriangleMsgs_count > 0)
	{
		std::string msgString("v1 triangles ");
		msgString += std::to_string(BufferedTriangleMsgs_count);
		msgString += " dim 3 ";
		msgString += BufferedTriangleMsgs.str();

		//socket->send( zmq::const_buffer(msgString.c_str(),msgString.size()) );
	}

	InitBuffers();
}
