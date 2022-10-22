#pragma once

#include "../../extern/mpi_wrapper/src/include.hpp"
#include "../Agents/AbstractAgent.hpp"
#include "../Agents/NucleusAgent.hpp"
#include "../config.hpp"
#include "../util/Vector3d.hpp"
#include <array>
#include <memory>
#include <utility>

inline MPI_Datatype MPIw_PAIR_INT_BOOL;
inline MPI_Datatype MPIw_PAIR_INT_INT;
inline MPI_Datatype MPIw_AABB_WITH_INFO;
inline MPI_Datatype MPIw_VECTOR3D_FLOAT;
inline MPI_Datatype MPIw_VECTOR3D_DOUBLE;

class AABBwithInfo {
  public:
	int agent_id;
	std::size_t agent_type_id;
	int geometry_version;
	AxisAlignedBoundingBox AABB;
};

class SerializedNucleusAgent {
  public:
	SerializedNucleusAgent() = default;
	SerializedNucleusAgent(const NucleusAgent& ag)
	    : ID(ag.getID()), type(ag.getAgentType()),
	      sphere_centres(
	          dynamic_cast<const Spheres&>(ag.getGeometry()).getCentres()),
	      sphere_radii(
	          dynamic_cast<const Spheres&>(ag.getGeometry()).getRadii()),
	      geom_version(ag.getGeometry().version) {}

	int ID;
	std::string type;

	std::vector<Vector3d<config::geometry::precision_t>> sphere_centres;
	std::vector<config::geometry::precision_t> sphere_radii;
	int geom_version;

	std::unique_ptr<ShadowAgent> createShadowCopy() const {
		Spheres _sph{sphere_centres, sphere_radii};
		_sph.version = geom_version;
		return std::make_unique<ShadowAgent>(
		    std::make_unique<Spheres>(std::move(_sph)), ID, type);
	}
};

class MPIManager {
  public:
	using _pair_int_bool = std::pair<int, bool>;
	using _pair_int_int = std::pair<int, int>;
	MPIManager() {
		init_pair<_pair_int_bool>(&MPIw_PAIR_INT_BOOL);
		init_pair<_pair_int_int>(&MPIw_PAIR_INT_INT);
		init_general<AABBwithInfo>(&MPIw_AABB_WITH_INFO);
		init_general<Vector3d<float>>(&MPIw_VECTOR3D_FLOAT);
		init_general<Vector3d<double>>(&MPIw_VECTOR3D_DOUBLE);
	}

	~MPIManager() {
		MPI_Type_free(&MPIw_PAIR_INT_BOOL);
		MPI_Type_free(&MPIw_PAIR_INT_INT);
		MPI_Type_free(&MPIw_AABB_WITH_INFO);
		MPI_Type_free(&MPIw_VECTOR3D_FLOAT);
		MPI_Type_free(&MPIw_VECTOR3D_DOUBLE);
	}

  private:
	template <typename pair_t>
	void init_pair(MPI_Datatype* out_type) {
		using first_t = pair_t::first_type;
		using second_t = pair_t::second_type;

		std::array lengths = {1, 1};
		std::array displs = {MPI_Aint(offsetof(pair_t, first)),
		                     MPI_Aint(offsetof(pair_t, second))};
		std::array types = {MPIw::types::get_mpi_type<first_t>(),
		                    MPIw::types::get_mpi_type<second_t>()};

		MPI_Type_create_struct(2, lengths.data(), displs.data(), types.data(),
		                       out_type);
		MPI_Type_create_resized(*out_type, MPI_Aint(0),
		                        MPI_Aint(sizeof(pair_t)), out_type);

		MPI_Type_commit(out_type);
	}

	template <typename T>
	void init_general(MPI_Datatype* out_type) {
		MPI_Type_contiguous(sizeof(T), MPI_BYTE, out_type);
		MPI_Type_commit(out_type);
	}

	// ================ Variables ================

	MPIw::Init_threads_raii _mpi{nullptr, nullptr, MPI_THREAD_MULTIPLE};
};

MPIw_register_type(typename ::MPIManager::_pair_int_bool, MPIw_PAIR_INT_BOOL);
MPIw_register_type(typename ::MPIManager::_pair_int_int, MPIw_PAIR_INT_INT);
MPIw_register_type(AABBwithInfo, MPIw_AABB_WITH_INFO);
MPIw_register_type(Vector3d<float>, MPIw_VECTOR3D_FLOAT);
MPIw_register_type(Vector3d<double>, MPIw_VECTOR3D_FLOAT);

inline constexpr int RANK_DIRECTOR = 0;

enum class async_tags { request_shadow_agent, send_shadow_agent, shutdown };