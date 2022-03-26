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
	                                const Vector3d<G_FLOAT>& vxSize,
	                                const size_t noOfParticlesPerVoxel = 10);
};
