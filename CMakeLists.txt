#------------------------------------------------------------------------
#
# Cross platform CMake configure script for 'EmbryoGen' simulator
#
# authors: Vladimir Ulman
#          Max Planck Institute of Cell Biology and Genetics (c) 2018
#
#------------------------------------------------------------------------

project(EmbryoGen)
cmake_minimum_required(VERSION 3.1)

include(GNUInstallDirs)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE STRING "Tracing the compilation process" FORCE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wconversion")

option(DEBUG_VERSION "Shall I display debug information?" OFF)
if (DEBUG_VERSION)
		add_definitions(-DDEBUG)
		set(CMAKE_BUILD_TYPE "DEBUG" CACHE STATIC "" FORCE)
else (DEBUG_VERSION)
		set(CMAKE_BUILD_TYPE "RELEASE" CACHE STATIC "" FORCE)
endif (DEBUG_VERSION)

#-------------------------
# USE STATIC LIBS OR NOT?
#-------------------------

option(SEARCH_ALL_DEPLIBS     "Shall all dependency libraries be explicitly linked? Useful mainly for static builds." OFF)
option(SEARCH_FOR_STATIC      "Shall CMAKE look for static variants of the dependency libraries?" OFF)

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

#-----------------------------------
# TARGET COMPILATION - SOURCE FILES
#-----------------------------------

set(SOURCES
		src/util/Vector3d.cpp
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
		src/Agents/util/Texture.cpp
		src/Simulation.cpp
		src/main.cpp)

file(GLOB SCENARIOSOURCES src/Scenarios/*.cpp)

add_executable(embryogen ${SOURCES} ${SCENARIOSOURCES})

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

# cppzmq header only (that C++-ish wraps around the original libzmq)
find_path(INC_CPPZMQ "zmq.hpp" DOC "EmbryoGen, being a C++ project, needs cppzmq (zmq.hpp) installed around libzmq.")
include_directories(${INC_CPPZMQ})

# ImgTransfer code
find_path(INC_DAISwp13 "TransferImage.h" DOC "Locate TransferImage.h from the C++ flavour of the DAIS-wp13 images transfer project.")
find_library(LIB_DAISwp13 DAIS-wp13.a)
include_directories(${INC_DAISwp13})
set(LIBS ${LIBS} ${LIB_DAISwp13})

#----------------------------------------------
# TARGET LINKING - FURTHER-LEVEL REQUIRED LIBS
#----------------------------------------------

if (SEARCH_ALL_DEPLIBS)
		# explicitly add libraries required by the CBIA i3dlibs
		find_library(LIB_LAPACK lapack)
		find_library(LIB_BLAS blas)
		set(LIBS ${LIBS} ${LIB_LAPACK} ${LIB_BLAS})

		find_library(LIB_TIFF tiff)
		find_library(LIB_ICS ics)
		set(LIBS ${LIBS} ${LIB_TIFF} ${LIB_ICS})

		find_library(LIB_Z z)
		set(LIBS ${LIBS} ${LIB_Z})

		find_library(LIB_FFTW_SINGLE_THREADS fftw3f_threads)
		find_library(LIB_FFTW_SINGLE fftw3f)
		find_library(LIB_FFTW_DOUBLE_THREADS fftw3_threads)
		find_library(LIB_FFTW_DOUBLE fftw3)
		find_library(LIB_FFTW_LDOUBLE_THREADS fftw3l_threads)
		find_library(LIB_FFTW_LDOUBLE fftw3l)
		set(LIBS ${LIBS} ${LIB_FFTW_SINGLE_THREADS} ${LIB_FFTW_SINGLE}
					${LIB_FFTW_DOUBLE_THREADS} ${LIB_FFTW_DOUBLE}
					${LIB_FFTW_LDOUBLE_THREADS} ${LIB_FFTW_LDOUBLE})
endif (SEARCH_ALL_DEPLIBS)

#----------------
# TARGET LINKING
#----------------
target_link_libraries(embryogen ${LIBS} libzmq-static)
