#------------------------------------------------------------------------
#
# Cross platform CMake configure script for 'TRAgen' project
#
# authors: Vladimir Ulman, Zoltan Oremus, David Svoboda
#          CBIA, Masaryk University (c) 2015
#
#------------------------------------------------------------------------

project(TRAgen)

cmake_minimum_required(VERSION 3.1)
include(GNUInstallDirs)
set (CMAKE_CXX_STANDARD 11)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE STRING
		  "Tracing the compilation process" FORCE)

#
# set the debug/release version
#
option(DEBUG_VERSION "Shall I add debug information?" OFF)

if (DEBUG_VERSION)
		  add_definitions(-DTRAGEN_DEBUG)
		  set(CMAKE_BUILD_TYPE "DEBUG" CACHE STATIC "" FORCE)
		  if (WIN32)
					 set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
		  endif (WIN32)
else (DEBUG_VERSION)
		  set(CMAKE_BUILD_TYPE "RELEASE" CACHE STATIC "" FORCE)
		  if (WIN32)
					 set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
		  endif (WIN32)
endif (DEBUG_VERSION)

#----------------
# COMPILER FLAGS
#----------------

#
# enable warnings
#
if (WIN32)	
		  add_definitions(-DGSL_DLL -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -DFREEGLUT_LIB_PRAGMAS=0)
		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /nologo /wd4251 /wd4275")
else (WIN32)
		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wconversion")
endif (WIN32)


#---------------
# REQUIRED LIBS
#---------------

find_path(INC_I3D "i3d/image3d.h")
include_directories(${INC_I3D} ${INC_I3D}/i3d)
find_library(LIB_I3DCORE "i3dcore")
find_library(LIB_I3DALGO "i3dalgo")
set(LIBS ${LIBS} ${LIB_I3DALGO} ${LIB_I3DCORE})
if (UNIX AND CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -defsym -Xlinker MAIN__=main")
endif (UNIX AND CMAKE_COMPILER_IS_GNUCXX)


# GSL library
find_library(LIB_GSL gsl)
set(LIBS ${LIBS} ${LIB_GSL})
find_library(LIB_GSLCBLAS gslcblas)
set(LIBS ${LIBS} ${LIB_GSLCBLAS})
find_path(INC_GSL "gsl/gsl_randist.h")
include_directories(${INC_GSL})


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
		  src/simulation.cpp
		  src/agents.cpp
		  src/rnd_generators.cpp
		  src/main.cpp
		  src/DisplayUnits/SceneryDisplayUnit.cpp
		  src/DisplayUnits/SceneryBufferedDisplayUnit.cpp)

#---------
# TARGETS
#---------
add_executable(tragen ${SOURCES})
target_link_libraries(tragen ${LIBS} libzmq-static)
