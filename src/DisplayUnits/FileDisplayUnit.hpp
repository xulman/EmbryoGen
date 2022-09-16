#pragma once

#include <fstream>
#include "DisplayUnit.hpp"
#include "../util/report.hpp"

/**
 * This class implements drawing by reporting what is being drawed
 * into a file, in a (human-readable) textual form.
 *
 * Author: Vladimir Ulman, 2019
 */
class FileDisplayUnit : public DisplayUnit
{
public:
	FileDisplayUnit(const char* filename)
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
			logger.open(filename);  //the original filename
		else
			logger.open(fn);        //the last "versioned" filename
	}

	~FileDisplayUnit()
	{
		logger.close();
	}

	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) override
	{
		logger << "Point ID " << ID << ": "
		       << pos << ", radius=" << radius << ", color=" << color << "\n";
	}

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override
	{
		logger << "Line ID " << ID << ": "
		       << posA << " <-> " << posB << ", color=" << color << "\n";
	}

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override
	{
		logger << "Vector ID " << ID << ": "
		       << "(" << vector.x << "," << vector.y << "," << vector.z << ")"
		       << " @ " << pos << ", color=" << color << "\n";
	}

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override
	{
		logger << "Triangle ID " << ID << ": "
		       << posA << ", " << posB << ", " << posC << ", color=" << color << "\n";
	}

	void Tick(const std::string& msg) override
	{
		logger << "Tick " << msg << "\n";
	}

	void Flush(void) override
	{
		logger.flush();
	}

protected:
	std::ofstream logger;
};
