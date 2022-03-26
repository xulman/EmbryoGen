#pragma once

#include "../util/report.hpp"
#include "DisplayUnit.hpp"

/**
 * This class implements drawing by reporting what is being drawed
 * on the console, that is, in a textual form.
 *
 * Author: Vladimir Ulman, 2018
 */
class ConsoleDisplayUnit : public DisplayUnit {
  public:
	void DrawPoint(const int ID,
	               const Vector3d<float>& pos,
	               const float radius = 1.0f,
	               const int color = 0) override {
		report::message(fmt::format("ID {}: {}, radius={}, color={}", ID,
		                            toString(pos), radius, color));
	}

	void DrawLine(const int ID,
	              const Vector3d<float>& posA,
	              const Vector3d<float>& posB,
	              const int color = 0) override {
		report::message(fmt::format("ID {}: {} <-> {}, color={}", ID,
		                            toString(posA), toString(posB), color));
	}

	void DrawVector(const int ID,
	                const Vector3d<float>& pos,
	                const Vector3d<float>& vector,
	                const int color = 0) override {
		report::message(fmt::format("ID {}: {} @ {}, color={}", ID,
		                            toString(vector), toString(pos), color));
	}

	void DrawTriangle(const int ID,
	                  const Vector3d<float>& posA,
	                  const Vector3d<float>& posB,
	                  const Vector3d<float>& posC,
	                  const int color = 0) override {
		report::message(fmt::format("ID {}: {}, {}, {}, color={}", ID,
		                            toString(posA), toString(posB),
		                            toString(posC), color));
	}

	void Tick(const std::string& msg) override { report::message(msg); }
};
