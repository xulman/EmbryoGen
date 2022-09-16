#pragma once

#include "../tools/structures/SmallVector.hpp"
#include "DisplayUnit.hpp"
#include <memory>

/**
 * This class is essentially a repeater that repeats the drawing
 * commands on all 'DisplayUnit's that are registered with an
 * instance of this class.
 *
 * Author: Vladimir Ulman, 2018
 */
class BroadcasterDisplayUnit : public DisplayUnit {
  public:
	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) override {
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawPoint(ID, pos, radius, color);
	}

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override {
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawLine(ID, posA, posB, color);
	}

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override {
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawVector(ID, pos, vector, color);
	}

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override {
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->DrawTriangle(ID, posA, posB, posC, color);
	}

	void Flush(void) override {
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->Flush();
	}

	void Tick(const std::string& msg) override {
		for (auto it = displayUnits.begin(); it != displayUnits.end(); ++it)
			(*it)->Tick(msg);
	}

	/// variants with pointer params
	void RegisterUnit(std::shared_ptr<DisplayUnit> ds) {
		displayUnits.push_back(std::move(ds));
	}

	void UnregisterUnit(std::shared_ptr<DisplayUnit> ds)
	{
		UnregisterUnit(ds.get());
	}

	void UnregisterUnit(DisplayUnit* ds) {
		for (std::size_t i = 0; i < displayUnits.size(); ++i) {
			if (displayUnits[i].get() == ds)
				displayUnits.erase_at(i);
		}
	}

  private:
	/** local list of who/where to send the drawing requests */
	tools::structures::SmallVector5<std::shared_ptr<DisplayUnit>> displayUnits;
};
