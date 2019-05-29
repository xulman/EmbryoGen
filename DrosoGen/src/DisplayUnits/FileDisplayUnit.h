#ifndef FILEDISPLAYUNIT_H
#define FILEDISPLAYUNIT_H

#include <fstream>
#include "DisplayUnit.h"

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
		logger.open(filename);
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

	void Tick(const char* msg) override
	{
		logger << "Tick " << (msg != NULL ? msg : "(no message given)") << "\n";
	}

	void Flush(void) override
	{
		logger.flush();
	}

protected:
	std::ofstream logger;
};
#endif
