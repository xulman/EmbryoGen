#------------------------------------------------------------------------
#
# Cross platform CMake configure script for 'TRAgen' project
#
# authors: Vladimir Ulman, Zoltan Oremus, David Svoboda
#          CBIA, Masaryk University (c) 2015
#
#------------------------------------------------------------------------

project(TRAgen)

cmake_minimum_required(VERSION 2.6) 

set(CMAKE_VERBOSE_MAKEFILE ON CACHE STRING
		  "Tracing the compilation process" FORCE)

#
# set the debug/release version
#
option(DEBUG_VERSION "Shall I add debug information?" ON)

if (DEBUG_VERSION)
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

#
# enabling OpenMP architecure
#
if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp")
else (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif (WIN32)

#
# options
#
option(RENDER_IMAGES "Shall I generate the bitmap data?" ON)

if (RENDER_IMAGES)
		  ADD_DEFINITIONS(-DRENDER_TO_IMAGES)
endif (RENDER_IMAGES)

option(READ_USER_DEFINED_TEXTURES "Shall I include texture data?" ON)

if (READ_USER_DEFINED_TEXTURES)
		  ADD_DEFINITIONS(-DREAD_TEXTURES)
endif (READ_USER_DEFINED_TEXTURES)

#
# required libs - i3d 
#

# conditionally
if (RENDER_IMAGES OR READ_USER_DEFINED_TEXTURES)
	find_path(INC_I3D "i3d/image3d.h")
	include_directories(${INC_I3D} ${INC_I3D}/i3d)

	find_library(LIB_I3DCORE "i3dcore")
	find_library(LIB_I3DALGO "i3dalgo")
	set(LIBS ${LIBS} ${LIB_I3DALGO} ${LIB_I3DCORE})
endif (RENDER_IMAGES OR READ_USER_DEFINED_TEXTURES)

# alwasy
find_library(LIB_GLUT "glut")
find_library(LIB_GL "GL")
find_library(LIB_GLEW "GLEW")
find_library(LIB_GSL "gsl")
find_library(LIB_GSLCBLAS "gslcblas")
set(LIBS ${LIBS} ${LIB_GL} ${LIB_GLUT} ${OPENGL_LIBRARIES} ${LIB_GLEW} ${LIB_GSL} ${LIBGSLCBLAS})

#
# source files
#
set(SOURCES 
		  src/graphics.cpp  
		  src/simulation.cpp
		  src/agents.cpp
		  src/rnd_generators.cpp
		  src/main.cpp)

# 
# targets
#
add_executable(tragen ${SOURCES})
target_link_libraries(tragen ${LIBS})
