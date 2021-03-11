#------------------------------------------------------------------------
#
# Cross platform CMake configure script for 'EmbryoGen' simulator
#
# authors: Vladimir Ulman, Pavel Moravec
#          Max Planck Institute of Cell Biology and Genetics (c) 2018
#          IT4Innovations National Supercomputing Center, VSB-TUO (c) 2020
#
#------------------------------------------------------------------------

project(EmbryoGen)
cmake_minimum_required(VERSION 3.1)

include(GNUInstallDirs)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE STRING "Tracing the compilation process" FORCE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wconversion")

#------------------------------------------------
# ALL OPTIONS ARE SUMMARIZED HERE (CONTROL PANEL)
#------------------------------------------------

option(DEBUG_VERSION      "Shall I display debug information?" OFF)
option(DEBUG_DIST_VERSION "Shall I display debug information for distributed communication?" OFF)

option(FEATURE_ENABLEPHOTOBLEACHING  "Shall texture particles be loosing their intensity in the phantom image?" OFF)
option(FEATURE_USEFILOGENSYNTHOSCOPY "Shall synthoscopy code from FiloGen be used instead of the one from MitoGen?" ON)
option(FEATURE_RUNDISTRIBUTED        "Shall the simulation use MPI (message passing interface) and run distributed?" OFF)
option(FEATURE_USEOpenMP             "Use OpenMP for single node parallelization" OFF)

option(SEARCH_ALL_DEPLIBS     "Shall all dependency libraries be explicitly linked? Useful mainly for static builds." OFF)
option(SEARCH_FOR_STATIC      "Shall CMAKE look for static variants of the dependency libraries?" OFF)

#-------------------------------------------------------
# TRANSLATE FEATURE REQUESTS INTO PREPROCESSOR CONSTANTS
#-------------------------------------------------------

if (DEBUG_VERSION)
	add_definitions(-DDEBUG)
endif (DEBUG_VERSION)

if (DEBUG_DIST_VERSION)
	add_definitions(-DDISTRIBUTED_DEBUG)
endif (DEBUG_DIST_VERSION)

if (FEATURE_ENABLEPHOTOBLEACHING)
	add_definitions(-DENABLED_PHOTOBLEACHING)
endif (FEATURE_ENABLEPHOTOBLEACHING)

if (FEATURE_USEFILOGENSYNTHOSCOPY)
	add_definitions(-DENABLE_FILOGEN_PHASEIIandIII)
else (FEATURE_USEFILOGENSYNTHOSCOPY)
	add_definitions(-DENABLE_MITOGEN_FINALPREVIEW)
endif (FEATURE_USEFILOGENSYNTHOSCOPY)

if (FEATURE_RUNDISTRIBUTED)
	add_definitions(-DDISTRIBUTED)
endif (FEATURE_RUNDISTRIBUTED)

#-------------------------
# USE STATIC LIBS OR NOT?
#-------------------------

if (SEARCH_FOR_STATIC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	set(CMAKE_LIBRARY_PATH "/reset/path/to/myLibs/" CACHE PATH "optional additional search path where static libs might be gathered")
	set(INC_I3D "${CMAKE_LIBRARY_PATH}/../include" CACHE PATH "additional search path where static includes should be found")
	set(INC_GSL "${CMAKE_LIBRARY_PATH}/../include" CACHE PATH "additional search path where static includes should be found")
else (SEARCH_FOR_STATIC)
	if (APPLE)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".dylib")
	else (APPLE)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
		# dynamic i3dalgo uses the f2c library for which
		# the following tweak must be here
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -defsym -Xlinker MAIN__=main")
	endif (APPLE)
endif (SEARCH_FOR_STATIC)

#-------------------------
# TARGET COMPILATION SETUP
#-------------------------

if (DEBUG_VERSION)
	set(CMAKE_BUILD_TYPE "DEBUG" CACHE STATIC "" FORCE)
else (DEBUG_VERSION)
	set(CMAKE_BUILD_TYPE "RELEASE" CACHE STATIC "" FORCE)
endif (DEBUG_VERSION)

if (FEATURE_RUNDISTRIBUTED)
	find_package(MPI REQUIRED)
	message(STATUS "===============================================================================")
	message(STATUS "==> MPI Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")
	message(STATUS "===============================================================================")
	set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif (FEATURE_RUNDISTRIBUTED)

#-----------------------------------
# TARGET COMPILATION - SOURCE FILES
#-----------------------------------

set(SOURCES
		src/util/Vector3d.cpp
		src/util/strings.cpp
		src/util/rnd_generators.cpp
		src/util/flowfields.cpp
		src/util/report.cpp
		src/util/texture/perlin.cpp
		src/util/texture/myround.cpp
		src/util/texture/texture.cpp
		src/util/synthoscopy/finalpreview.cpp
		src/util/synthoscopy/FiloGen_VM.cpp
		src/util/synthoscopy/SNR.cpp
		src/util/AgentsMap.cpp
		src/DisplayUnits/SceneryDisplayUnit.cpp
		src/DisplayUnits/SceneryBufferedDisplayUnit.cpp
		src/Geometries/Geometry.cpp
		src/Geometries/Spheres.cpp
		src/Geometries/ScalarImg.cpp
		src/Geometries/VectorImg.cpp
		src/Geometries/Mesh.cpp
		src/Agents/util/CellCycle.cpp
		src/Agents/util/Texture.cpp
		src/Agents/util/TextureFunctions.cpp
		src/Agents/NucleusAgent.cpp
		src/Agents/Nucleus4SAgent.cpp
		src/Agents/NucleusNSAgent.cpp
		src/Agents/ShapeHinter.cpp
		src/Agents/TrajectoriesHinter.cpp
		src/Scenarios/common/Scenario.cpp
		src/TrackRecord.cpp
		src/Communication/DistributedCommunicator.cpp
		src/Director.cpp
		src/FrontOfficer.cpp
		src/main.cpp)

if (FEATURE_RUNDISTRIBUTED)
	set(D_FO_SOURCES
		src/Communication/DirectorMPI.cpp
		src/Communication/FrontOfficerMPI.cpp)
else (FEATURE_RUNDISTRIBUTED)
	set(D_FO_SOURCES
		src/Communication/DirectorSMP.cpp
		src/Communication/FrontOfficerSMP.cpp)
endif (FEATURE_RUNDISTRIBUTED)

file(GLOB SCENARIOSOURCES src/Scenarios/*.cpp)

add_executable(embryogen ${SOURCES} ${D_FO_SOURCES} ${SCENARIOSOURCES})
add_custom_target(commitHash ALL "../imprintGitHash.sh")

#--------------------------------------------
# TARGET LINKING - FIRST-LEVEL REQUIRED LIBS
#--------------------------------------------

# i3dlibs library
find_path(INC_I3D "i3d/image3d.h")
find_library(LIB_I3DCORE "i3dcore")
find_library(LIB_I3DALGO "i3dalgo")
include_directories(${INC_I3D} ${INC_I3D}/i3d)
set(LIBS ${LIBS} ${LIB_I3DALGO} ${LIB_I3DCORE})
if (UNIX AND CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -defsym -Xlinker MAIN__=main")
endif (UNIX AND CMAKE_COMPILER_IS_GNUCXX)

# GSL library
find_path(INC_GSL "gsl/gsl_randist.h")
find_library(LIB_GSL gsl)
find_library(LIB_GSLCBLAS gslcblas)
include_directories(${INC_GSL})
set(LIBS ${LIBS} ${LIB_GSL} ${LIB_GSLCBLAS})


# attempts to find the complete ZeroMQ Cmake package because...
# a) default installation provides the Cmake module files
# b) and the module knows how to link ZeroMQ statically to my libraries/binaries
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/cmake/ZeroMQ")
find_package(ZeroMQ)

# libzmq autotools install: fallback to pkg-config
if(NOT ZeroMQ_FOUND)
	# try again with pkg-config (normal install of zeromq)
	list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/libzmq-pkg-config)
	find_package(ZeroMQ REQUIRED)
endif()
set(LIBS ${LIBS} libzmq-static)

# cppzmq header only (that C++-ish wraps around the original libzmq)
find_path(INC_CPPZMQ "zmq.hpp" DOC "EmbryoGen, being a C++ project, needs cppzmq (zmq.hpp) installed around libzmq.")
include_directories(${INC_CPPZMQ})

# ImgTransfer code
find_path(INC_DAISwp13 "TransferImage.h" DOC "Locate TransferImage.h from the C++ flavour of the DAIS-wp13 images transfer project.")
find_library(LIB_DAISwp13 DAIS-wp13.a)
include_directories(${INC_DAISwp13})
set(LIBS ${LIBS} ${LIB_DAISwp13})

if (FEATURE_RUNDISTRIBUTED)
	set(LIBS ${LIBS} MPI::MPI_CXX)
endif (FEATURE_RUNDISTRIBUTED)

#----------------------------------------------
# TARGET LINKING - FURTHER-LEVEL REQUIRED LIBS
#----------------------------------------------

if (SEARCH_ALL_DEPLIBS)
		# explicitly add libraries required by the CBIA i3dlibs
		find_library(LIB_LAPACK lapack)
		find_library(LIB_BLAS blas)
		set(LIBS ${LIBS} ${LIB_LAPACK} ${LIB_BLAS})

		find_library(LIB_TIFF tiff)
		find_library(LIB_JPEG jpeg)
		find_library(LIB_ICS ics)
		set(LIBS ${LIBS} ${LIB_TIFF} ${LIB_ICS} ${LIB_JPEG})

		find_library(LIB_Z z)
		set(LIBS ${LIBS} ${LIB_Z})

		find_library(LIB_FFTW_SINGLE_THREADS fftw3f_threads)
		find_library(LIB_FFTW_SINGLE fftw3f)
		find_library(LIB_FFTW_DOUBLE_THREADS fftw3_threads)
		find_library(LIB_FFTW_DOUBLE fftw3)
		find_library(LIB_FFTW_LDOUBLE_THREADS fftw3l_threads)
		find_library(LIB_FFTW_LDOUBLE fftw3l)
		set(LIBS ${LIBS} ${LIB_FFTW_SINGLE_THREADS}
		         ${LIB_FFTW_SINGLE}
		         ${LIB_FFTW_DOUBLE_THREADS}
		         ${LIB_FFTW_DOUBLE}
		         ${LIB_FFTW_LDOUBLE_THREADS}
		         ${LIB_FFTW_LDOUBLE})
endif (SEARCH_ALL_DEPLIBS)

#----------------
# TARGET LINKING
#----------------

if(FEATURE_USEOpenMP)
	if (APPLE)
		if(CMAKE_C_COMPILER_ID MATCHES "Clang")
			set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
			set(OpenMP_CXX_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument")
			set(OpenMP_CXX_LIB_NAMES "libomp")
			find_library(OpenMP_libomp_LIBRARY omp)
			set(LIBS ${LIBS} OpenMP_libomp_LIBRARY)
		endif()
	endif (APPLE)

	find_package(OpenMP)
	set(LIBS ${LIBS} OpenMP::OpenMP_CXX)
endif(FEATURE_USEOpenMP)

target_link_libraries(embryogen ${LIBS})
