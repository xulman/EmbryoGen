#------------------------------------------------------------------------
#
# Cross platform CMake configure script for 'DrosoGen' simulator
#
# authors: Vladimir Ulman
#          Max Planck Institute of Cell Biology and Genetics (c) 2018
#
#------------------------------------------------------------------------

project(DrosoGen)
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


#---------------
# REQUIRED LIBS
#---------------

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

#--------------
# SOURCE FILES
#--------------

set(SOURCES
		  src/DisplayUnits/SceneryDisplayUnit.cpp
		  src/DisplayUnits/SceneryBufferedDisplayUnit.cpp
		  src/util/rnd_generators.cpp
		  src/Simulation.cpp
		  src/main.cpp)

#---------
# TARGETS
#---------
add_executable(drosogen ${SOURCES})
target_link_libraries(drosogen ${LIBS} libzmq-static)
