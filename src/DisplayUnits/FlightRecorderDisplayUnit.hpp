#ifndef FLIGHTRECORDERDISPLAYUNIT_H
#define FLIGHTRECORDERDISPLAYUNIT_H

#include <fstream>
#include "DisplayUnit.hpp"
#include "../util/report.hpp"

/**
 * This class implements drawing by reporting what is being drawed
 * into a file, in a form of messages agreed with the SimViewer.
 * SimViewer can later replay the messages.
 *
 * Author: Vladimir Ulman, 2019
 */
class FlightRecorderDisplayUnit : public DisplayUnit
{
public:
	FlightRecorderDisplayUnit(const char* filename)
	{
		char fn[1024];

		int tryCnt = 0;
		std::ifstream testingFile(filename);

		while (testingFile.is_open() && tryCnt < 100)
		{
			//try another "version" of the filename
			++tryCnt;
			sprintf(fn,"%s~%d", filename, tryCnt);

			testingFile.close();
			testingFile.open(fn);
		}

		if (testingFile.is_open())
		{
			testingFile.close();
throw report::rtError("Refusing to create 101st flight record, please clean up your disk.");
		}

		if (tryCnt == 0)
		{
			logger.open(filename);  //the original filename
report::debugMessage(fmt::format("Recording into: {}" , filename));
		}
		else
		{
			logger.open(fn);        //the last "versioned" filename
report::debugMessage(fmt::format("Recording into: {}" , fn));
		}
	}

	~FlightRecorderDisplayUnit()
	{
		logger.close();
	}

	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) override
	{
		//message syntax := v1 points N dim D Point1 ... PointN
		//  Point syntax := ID x1 ... xD radius color
		//
		//where N is the number of points in this message,
		//where D is the dimension of one point,
		//where x1,...,xD,radius are real scalars, ID,color are integer scalars

		logger << "v1 points 1 dim 3 " << ID << " "
		       << pos.x << " " << pos.y << " " << pos.z << " "
		       << radius << " " << color << "\n";
	}


	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override
	{
		//  message syntax := v1 lines N dim D PointPair1 ... PointPairN
		//PointPair syntax := ID o1 ... oD p1 ... pD color
		//
		//where N is the number of lines in this message,
		//where D is the dimension of one line end point,
		//where o1,...,oD,p1,...,pD are real scalars, ID,color are integer scalars
		//
		// o,p are two end points of a line

		logger << "v1 lines 1 dim 3 " << ID << " "
		       << posA.x << " " << posA.y << " " << posA.z << " "
		       << posB.x << " " << posB.y << " " << posB.z << " "
		       << color << "\n";
	}


	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override
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

		logger << "v1 vectors 1 dim 3 " << ID << " "
		       << pos.x    << " " << pos.y    << " " << pos.z    << " "
		       << vector.x << " " << vector.y << " " << vector.z << " "
		       << color << "\n";
	}


	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override
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

		logger << "v1 triangles 1 dim 3 " << ID << " "
		       << posA.x << " " << posA.y << " " << posA.z << " "
		       << posB.x << " " << posB.y << " " << posB.z << " "
		       << posC.x << " " << posC.y << " " << posC.z << " "
		       << color << "\n";
	}

	void Tick(const std::string& msg) override
	{
		logger << "v1 tick " << msg << "\n";
	}

	void Flush(void) override
	{
		logger.flush();
	}

protected:
	std::ofstream logger;
};
#endif
