#pragma once

#include "DisplayUnit.hpp"
#include <boost/container/small_vector.hpp>
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
		for (auto& du : displayUnits)
			du->DrawPoint(ID, pos, radius, color);
	}

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override {
		for (auto& du : displayUnits)
			du->DrawLine(ID, posA, posB, color);
	}

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override {
		for (auto& du : displayUnits)
			du->DrawVector(ID, pos, vector, color);
	}

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override {
		for (auto& du : displayUnits)
			du->DrawTriangle(ID, posA, posB, posC, color);
	}

	void Flush(void) override {
		for (auto& du : displayUnits)
			du->Flush();
	}

	void Tick(const std::string& msg) override {
		for (auto& du : displayUnits)
			du->Tick(msg);
	}

	/** notes:
	 * 1) extends ownership of the provided DisplayUnit also into this unit;
	 * 2) owing to the (intentional) copy-construction of the parameter 'ds',
	 *    repetitive insertions of otherwise the same unit will be stored
	 *    multiple times;
	 * 3) as a consequence, it permits to insert the same unit multiple times */
	void RegisterUnit(std::shared_ptr<DisplayUnit> ds) {
		displayUnits.push_back(std::move(ds));
		//NB: std::move() is not necessary here, albeit functional:
		//a new instance of shared_ptr is created inside the vector, this instance
		//is filled with 'ds' content (increasing ownership number), currently
		//std::move() will clear the 'ds' but the scope would "kill" 'ds' anyway
		//soon -- both decreases the ownership number, so the effect is the same
	}

	/** note: removes "only" the first occurrence of the provided DisplayUnit */
	void UnregisterUnit(std::shared_ptr<DisplayUnit>& ds) {
		for (std::size_t i = 0; i < displayUnits.size(); ++i) {
			if (displayUnits[i].get() == ds.get()) {
				displayUnits.erase(displayUnits.begin() + i);
				return;
			}
		}
	}

  private:
	/** local list of who/where to send the drawing requests */
	boost::container::small_vector<std::shared_ptr<DisplayUnit>, 5>
	    displayUnits;
};
