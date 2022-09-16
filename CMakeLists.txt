#------------------------------------------------------------------------
#
# Cross platform CMake configure script for 'EmbryoGen' simulator
#
# authors: Vladimir Ulman, Pavel Moravec
#          Max Planck Institute of Cell Biology and Genetics (c) 2018
#          IT4Innovations National Supercomputing Center, VSB-TUO (c) 2020
#
#------------------------------------------------------------------------
# OPTION FOR STATIC BUILD MUST BE SET BEFORE SPECIFYING PROJECT(...) 
#-------------------------------------------------------------------

option(BUILD_STATIC		"Insert dependencies directly to executable" OFF)
# set paht to appropriate vcpkg manifest file
if (BUILD_STATIC)
set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR}/vcpkg/static)
else(BUILD_STATIC)
set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR}/vcpkg/dynamic)
endif(BUILD_STATIC)

cmake_minimum_required(VERSION 3.1)
project(EmbryoGen)

include(GNUInstallDirs)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE STRING "Tracing the compilation process" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion")

#------------------------------------------------
# ALL OPTIONS ARE SUMMARIZED HERE (CONTROL PANEL)
#------------------------------------------------

option(DEBUG_VERSION      "Shall I display debug information?" ON)
option(DEBUG_DIST_VERSION "Shall I display debug information for distributed communication?" OFF)

option(FEATURE_ENABLEPHOTOBLEACHING  "Shall texture particles be loosing their intensity in the phantom image?" OFF)
option(FEATURE_USEFILOGENSYNTHOSCOPY "Shall synthoscopy code from FiloGen be used instead of the one from MitoGen?" ON)
option(FEATURE_RUNDISTRIBUTED        "Shall the simulation use MPI (message passing interface) and run distributed?" OFF)
option(FEATURE_USEOpenMP             "Use OpenMP for single node parallelization" OFF)

#-------------------------------------------------------
# TRANSLATE FEATURE REQUESTS INTO PREPROCESSOR CONSTANTS
#-------------------------------------------------------

if (NOT DEBUG_VERSION)
    add_definitions(-DNDEBUG)
endif (NOT DEBUG_VERSION)

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

#---PYTHON CMAKE INJECTION START---
set(STD_MODULES )
add_custom_target(std_modules
    COMMAND ${CMAKE_COMMAND} -E echo "Building system libraries"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

foreach(module IN LISTS STD_MODULES)
    add_custom_command(TARGET std_modules PRE_BUILD 
        COMMAND ${CMAKE_COMMAND} -E echo "${module}"
        COMMAND g++ -fmodules-ts -std=c++20 -c -x c++-system-header ${module}
        )
endforeach()

set(SOURCES ${SOURCES} 
src/Agents/Nucleus4SAgent.cpp
src/Agents/NucleusAgent.cpp
src/Agents/NucleusNSAgent.cpp
src/Agents/ShapeHinter.cpp
src/Agents/TrajectoriesHinter.cpp
src/Agents/util/CellCycle.cpp
src/Agents/util/Texture.cpp
src/Agents/util/TextureFunctions.cpp
src/Communication/DirectorMPI.cpp
src/Communication/DirectorSMP.cpp
src/Communication/DistributedCommunicator.cpp
src/Communication/FrontOfficerMPI.cpp
src/Communication/FrontOfficerSMP.cpp
src/Director.cpp
src/DisplayUnits/SceneryBufferedDisplayUnit.cpp
src/DisplayUnits/SceneryDisplayUnit.cpp
src/FrontOfficer.cpp
src/Geometries/Geometry.cpp
src/Geometries/Mesh.cpp
src/Geometries/ScalarImg.cpp
src/Geometries/Spheres.cpp
src/Geometries/VectorImg.cpp
src/main.cpp
src/Scenarios/common/Scenario.cpp
src/Scenarios/Scenario_AFewAgents.cpp
src/Scenarios/Scenario_dragAndRotate.cpp
src/Scenarios/Scenario_dragAndRotateWithTexture.cpp
src/Scenarios/Scenario_DrosophilaRandom.cpp
src/Scenarios/Scenario_DrosophilaRegular.cpp
src/Scenarios/Scenario_modelledDivision.cpp
src/Scenarios/Scenario_mpiDebug.cpp
src/Scenarios/Scenario_parallel.cpp
src/Scenarios/Scenario_pseudoDivision.cpp
src/Scenarios/Scenario_Tetris.cpp
src/Scenarios/Scenario_Texture.cpp
src/Scenarios/Scenario_withCellCycle.cpp
src/Scenarios/utilityScenario_doPhaseIIandIII.cpp
src/Scenarios/utilityScenario_PerlinShowCase.cpp
src/TrackRecord.cpp
src/util/flowfields.cpp
src/util/report.cpp
src/util/rnd_generators.cpp
src/util/strings.cpp
src/util/synthoscopy/FiloGen_VM.cpp
src/util/synthoscopy/finalpreview.cpp
src/util/synthoscopy/SNR.cpp
src/util/texture/myround.cpp
src/util/texture/perlin.cpp
src/util/texture/texture.cpp
src/util/Vector3d.cpp
)

set(DEPS 
std_modules
)

#---PYTHON CMAKE INJECTION END---

set(SOURCES_OLD
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


set(SOURCE_TO_REMOVE 
    src/Communication/DirectorMPI.cpp
    src/Communication/FrontOfficerMPI.cpp
    src/Communication/DirectorSMP.cpp
    src/Communication/FrontOfficerSMP.cpp)

list(REMOVE_ITEM SOURCES ${SOURCE_TO_REMOVE})

if (FEATURE_RUNDISTRIBUTED)
	set(D_FO_SOURCES
		src/Communication/DirectorMPI.cpp
		src/Communication/FrontOfficerMPI.cpp)
else (FEATURE_RUNDISTRIBUTED)
	set(D_FO_SOURCES
		src/Communication/DirectorSMP.cpp
		src/Communication/FrontOfficerSMP.cpp)
endif (FEATURE_RUNDISTRIBUTED)

add_executable(embryogen ${SOURCES} ${D_FO_SOURCES})
add_dependencies(embryogen ${DEPS})
add_custom_target(commitHash ALL "../imprintGitHash.sh")
set_target_properties(embryogen PROPERTIES CXX_STANDARD 20)
#--------------------------------------------
# TARGET LINKING - FIRST-LEVEL REQUIRED LIBS
#--------------------------------------------

# i3dlibs library
if (BUILD_STATIC)
# static i3dlib dependencies
find_package(FFTW3 CONFIG REQUIRED)
set(LIBS ${LIBS} FFTW3::fftw3)

find_package(FFTW3f CONFIG REQUIRED)
set(LIBS ${LIBS} FFTW3::fftw3f)

find_package(FFTW3l CONFIG REQUIRED)
set(LIBS ${LIBS} FFTW3::fftw3l)

find_package(clapack CONFIG REQUIRED)
set(LIBS ${LIBS} lapack)

if  (UNIX)
if  (NOT APPLE)
	set(LIBS ${LIBS} f2c)
endif (NOT APPLE)
endif (UNIX)

find_package(OpenBLAS CONFIG REQUIRED)
set(LIBS ${LIBS} OpenBLAS::OpenBLAS)

find_package(TIFF REQUIRED)
set(LIBS ${LIBS} ${TIFF_LIBRARIES})

find_package(JPEG REQUIRED)
set(LIBS ${LIBS} ${JPEG_LIBRARIES})

find_package(libpng CONFIG REQUIRED)
if(WIN32)
	set(LIBS ${LIBS} png)
else(WIN32)
	set(LIBS ${LIBS} png_static)
endif(WIN32)

find_package(libics CONFIG REQUIRED)
set(LIBS ${LIBS} libics)

find_package(ZLIB REQUIRED)
set(LIBS ${LIBS} ZLIB::ZLIB)

find_package(hdf5 CONFIG REQUIRED)
if (WIN32)
	set(LIBS ${LIBS} hdf5::hdf5-shared hdf5::hdf5_hl-shared)
else(WIN32)
	set(LIBS ${LIBS} hdf5::hdf5-static hdf5::hdf5_hl-static)
endif(WIN32)

find_package(RXSpencer CONFIG REQUIRED)
set(LIBS ${LIBS} rxspencer)

add_library(i3dcore STATIC IMPORTED)
set_target_properties(i3dcore PROPERTIES
    IMPORTED_LOCATION /usr/local/lib/libi3dcore.a)

add_library(i3dalgo STATIC IMPORTED)
set_target_properties(i3dalgo PROPERTIES
    IMPORTED_LOCATION /usr/local/lib/libi3dalgo.a)

set(LIBS i3dcore i3dalgo ${LIBS})
else(BUILD_STATIC)
find_library(LIB_I3DCORE "i3dcore")
find_library(LIB_I3DALGO "i3dalgo")
set(LIBS ${LIBS} ${LIB_I3DALGO} ${LIB_I3DCORE})
endif(BUILD_STATIC)

# GSL library
find_package(GSL REQUIRED)
set(LIBS ${LIBS} GSL::gsl GSL::gslcblas)

# Google rpc
find_package(gRPC CONFIG REQUIRED)
set(LIBS ${LIBS} gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)

# FMT library
find_package(fmt CONFIG REQUIRED)
set(LIBS ${LIBS} fmt::fmt)

if (FEATURE_RUNDISTRIBUTED)
	set(LIBS ${LIBS} MPI::MPI_CXX)
endif (FEATURE_RUNDISTRIBUTED)

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
