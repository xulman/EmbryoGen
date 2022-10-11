#pragma once

#include "../../extern/mpi_wrapper/src/include.hpp"
#include <array>
#include <memory>
#include <utility>

static MPI_Datatype MPIw_PAIR_INT_BOOL;
static MPI_Datatype MPIw_PAIR_INT_INT;

class MPIManager {
  public:
	using _pair_int_bool = std::pair<int, bool>;
	using _pair_int_int = std::pair<int, int>;
	MPIManager() {
		// std::pair<int, bool>
		{
			std::array lengths = {1, 1};
			std::array displs = {MPI_Aint(offsetof(_pair_int_bool, first)),
			                     MPI_Aint(offsetof(_pair_int_bool, second))};
			std::array types = {MPIw::types::get_mpi_type<int>(),
			                    MPIw::types::get_mpi_type<bool>()};

			MPI_Type_create_struct(2, lengths.data(), displs.data(),
			                       types.data(), &MPIw_PAIR_INT_BOOL);
			MPI_Type_commit(&MPIw_PAIR_INT_BOOL);
		}

		// std::pair<int, int>
		{
			std::array lengths = {1, 1};
			std::array displs = {MPI_Aint(offsetof(_pair_int_int, first)),
			                     MPI_Aint(offsetof(_pair_int_int, second))};
			std::array types = {MPIw::types::get_mpi_type<int>(),
			                    MPIw::types::get_mpi_type<int>()};

			MPI_Type_create_struct(2, lengths.data(), displs.data(),
			                       types.data(), &MPIw_PAIR_INT_INT);
			MPI_Type_commit(&MPIw_PAIR_INT_INT);
		}
	}

	~MPIManager() {
		MPI_Type_free(&MPIw_PAIR_INT_BOOL);
		MPI_Type_free(&MPIw_PAIR_INT_INT);
	}

  private:
	MPIw::Init_raii _mpi{nullptr, nullptr};
};

MPIw_register_type(typename ::MPIManager::_pair_int_bool, MPIw_PAIR_INT_BOOL);
MPIw_register_type(typename ::MPIManager::_pair_int_int, MPIw_PAIR_INT_INT);

inline constexpr int RANK_DIRECTOR = 0;