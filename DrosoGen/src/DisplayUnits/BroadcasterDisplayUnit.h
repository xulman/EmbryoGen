#ifndef BROADCASTERDISPLAYUNIT_H
#define BROADCASTERDISPLAYUNIT_H

#include "DisplayUnit.h"
#include <forward_list>

/**
 * This class is essentially a repeater that repeats the drawing
 * commands on all 'DisplayUnit's that are registered with an
 * instance of this class.
 *
 * Author: Vladimir Ulman, 2018
 */
class BroadcasterDisplayUnit : public DisplayUnit
{
public:
	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0)
	{
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawPoint(ID,pos,radius,color);
	}

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0)
	{
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawLine(ID,posA,posB,color);
	}

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0)
	{
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawVector(ID,pos,vector,color);
	}

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0)
	{
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawTriangle(ID,posA,posB,posC,color);
	}

	void Flush(void)
	{
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->Flush();
	}


	///variants with reference params
	void RegisterUnit(DisplayUnit& ds)
	{
		displayUnits.push_front(&ds);
	}
	void UnregisterUnit(DisplayUnit& ds)
	{
		displayUnits.remove(&ds);
	}

	///variants with pointer params
	void RegisterUnit(DisplayUnit* ds)
	{
		displayUnits.push_front(ds);
	}
	void UnregisterUnit(DisplayUnit* ds)
	{
		displayUnits.remove(ds);
	}

private:
	/** local list of who/where to send the drawing requests */
	std::forward_list<DisplayUnit*> displayUnits;
};
#endif
