#pragma once

#include "../../Geometries/Spheres.hpp"
#include "../../util/Dots.hpp"

class TextureFunctions {
  public:
	template <typename FT>
	static void addTextureAlongLine(std::vector<Dot>& dots,
	                                const Vector3d<FT>& from,
	                                const Vector3d<FT>& to,
	                                const size_t noOfParticles = 5000) {
		const Vector3d<FT> delta = to - from;
		for (size_t i = 0; i < noOfParticles; ++i) {
			dots.emplace_back(from + ((FT)i / (FT)noOfParticles) * delta);
		}
	}

	template <typename FT>
	static void addTextureAlongGrid(std::vector<Dot>& dots,
	                                const Spheres& geom,
	                                const Vector3d<FT>& gridFrom,
	                                const Vector3d<FT>& gridStep,
	                                const Vector3d<float>& vxSize,
	                                const size_t noOfParticlesPerVoxel = 10) {
		Vector3d<float> pos;
		Vector3d<FT> lineStart;
		lineStart.from(geom.AABB.minCorner) += gridFrom;
		for (; lineStart.z < geom.AABB.maxCorner.z; lineStart.z += gridStep.z)
			for (lineStart.y = geom.AABB.minCorner.y + gridFrom.y;
			     lineStart.y < geom.AABB.maxCorner.y;
			     lineStart.y += gridStep.y) {
				pos.from(lineStart);
				for (; pos.x < geom.AABB.maxCorner.x; pos.x += vxSize.x)
					if (geom.collideWithPoint(pos) > -1)
						for (size_t i = 0; i < noOfParticlesPerVoxel; ++i)
							dots.emplace_back(pos);
			}

		lineStart.from(geom.AABB.minCorner) += gridFrom;
		for (; lineStart.z < geom.AABB.maxCorner.z; lineStart.z += gridStep.z)
			for (lineStart.x = geom.AABB.minCorner.x + gridFrom.x;
			     lineStart.x < geom.AABB.maxCorner.x;
			     lineStart.x += gridStep.x) {
				pos.from(lineStart);
				for (; pos.y < geom.AABB.maxCorner.y; pos.y += vxSize.y)
					if (geom.collideWithPoint(pos) > -1)
						for (size_t i = 0; i < noOfParticlesPerVoxel; ++i)
							dots.emplace_back(pos);
			}

		lineStart.from(geom.AABB.minCorner) += gridFrom;
		for (; lineStart.x < geom.AABB.maxCorner.x; lineStart.x += gridStep.x)
			for (lineStart.y = geom.AABB.minCorner.y + gridFrom.y;
			     lineStart.y < geom.AABB.maxCorner.y;
			     lineStart.y += gridStep.y) {
				pos.from(lineStart);
				for (; pos.z < geom.AABB.maxCorner.z; pos.z += vxSize.z)
					if (geom.collideWithPoint(pos) > -1)
						for (size_t i = 0; i < noOfParticlesPerVoxel; ++i)
							dots.emplace_back(pos);
			}
	}
};
