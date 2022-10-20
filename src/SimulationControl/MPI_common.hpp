#pragma once

#include "../../extern/mpi_wrapper/src/include.hpp"
#include "../Agents/AbstractAgent.hpp"
#include <array>
#include <memory>
#include <utility>

inline MPI_Datatype MPIw_PAIR_INT_BOOL;
inline MPI_Datatype MPIw_PAIR_INT_INT;
inline MPI_Datatype MPIw_AABB_WITH_INFO;

class AABBwithInfo {
  public:
	int agent_id;
	std::size_t agent_type_id;
	int geometry_version;
	AxisAlignedBoundingBox AABB;
};

class MPIManager {
  public:
	using _pair_int_bool = std::pair<int, bool>;
	using _pair_int_int = std::pair<int, int>;
	MPIManager() {
		init_pair<_pair_int_bool>(&MPIw_PAIR_INT_BOOL);
		init_pair<_pair_int_int>(&MPIw_PAIR_INT_INT);
		init_general<AABBwithInfo>(&MPIw_AABB_WITH_INFO);
	}

	~MPIManager() {
		MPI_Type_free(&MPIw_PAIR_INT_BOOL);
		MPI_Type_free(&MPIw_PAIR_INT_INT);
		MPI_Type_free(&MPIw_AABB_WITH_INFO);
	}

  private:
	MPIw::Init_raii _mpi{nullptr, nullptr};

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
};

MPIw_register_type(typename ::MPIManager::_pair_int_bool, MPIw_PAIR_INT_BOOL);
MPIw_register_type(typename ::MPIManager::_pair_int_int, MPIw_PAIR_INT_INT);
MPIw_register_type(AABBwithInfo, MPIw_AABB_WITH_INFO);

inline constexpr int RANK_DIRECTOR = 0;