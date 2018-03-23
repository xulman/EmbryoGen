#------------------------------------------------------------------------
#
# Cross platform CMake configure script for 'TRAgen' project
#
# authors: Vladimir Ulman, Zoltan Oremus, David Svoboda
#          CBIA, Masaryk University (c) 2015
#
#------------------------------------------------------------------------

project(TRAgen)

cmake_minimum_required(VERSION 3.0) 

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

#
# enable OpenMP architecure
#
if (WIN32)
		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp")
else (WIN32)
#		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif (WIN32)

#
# user defined options
#
option(O10-READ_USER_DEFINED_TEXTURES "Shall I include texture data?" ON)

if (O10-READ_USER_DEFINED_TEXTURES)
		  add_definitions(-DREAD_TEXTURES)
endif (O10-READ_USER_DEFINED_TEXTURES)

option(O20-RENDER_IMAGES "Shall I generate the bitmap data (phantoms and masks)?" ON)

if (O20-RENDER_IMAGES)
		  add_definitions(-DRENDER_TO_IMAGES)
endif (O20-RENDER_IMAGES)

option(O21-SIMULATE_FLUORESCENCE  "Shall I also generate synthetic fluorescence images (works only if O20-RENDER_IMAGES is ON)?" OFF)
option(O22-SIMULATE_PHASECONTRAST "Shall I also generate synthetic phase-contrast images (works only if O20-RENDER_IMAGES is ON)?" OFF)

if (O21-SIMULATE_FLUORESCENCE)
		  add_definitions(-DRENDER_FLUORESCENCE)
endif (O21-SIMULATE_FLUORESCENCE)
if (O22-SIMULATE_PHASECONTRAST)
		  add_definitions(-DRENDER_PHASECONTRAST)
endif (O22-SIMULATE_PHASECONTRAST)

option(O30-CELL_EXTRACTOR "Shall I compile tools to prepare texture data and cell configuration files?" OFF)

#---------------
# REQUIRED LIBS
#---------------

# conditionally
if (O20-RENDER_IMAGES OR O10-READ_USER_DEFINED_TEXTURES OR O30-CELL_EXTRACTOR)
	find_path(INC_I3D "i3d/image3d.h")
	include_directories(${INC_I3D} ${INC_I3D}/i3d)
	find_library(LIB_I3DCORE "i3dcore")
	find_library(LIB_I3DALGO "i3dalgo")
	set(LIBS ${LIBS} ${LIB_I3DALGO} ${LIB_I3DCORE})
	if (UNIX AND CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -defsym -Xlinker MAIN__=main")
	endif (UNIX AND CMAKE_COMPILER_IS_GNUCXX)
endif (O20-RENDER_IMAGES OR O10-READ_USER_DEFINED_TEXTURES OR O30-CELL_EXTRACTOR)

# GLUT library
find_library(LIB_GLUT "glut")
set(LIBS ${LIBS} ${LIB_GLUT})

find_path(INC_GLUT "GL/glut.h")
include_directories(${INC_GLUT})

# GLEW and OpenGL libraries
if (WIN32)
	set (LIB_OPENGL opengl32.lib)
   find_library(LIB_GLEW glew32)
else (WIN32)
	find_library(LIB_GLEW GLEW)
	find_library(LIB_OPENGL GL)
endif (WIN32)

set(LIBS ${LIBS} ${LIB_OPENGL} ${LIB_GLEW})
find_path(INC_GLEW "GL/glew.h")
include_directories(${INC_GLEW})

# GSL library
find_library(LIB_GSL gsl)
set(LIBS ${LIBS} ${LIB_GSL})
find_library(LIB_GSLCBLAS gslcblas)
set(LIBS ${LIBS} ${LIB_GSLCBLAS})
find_path(INC_GSL "gsl/gsl_randist.h")
include_directories(${INC_GSL})

#--------------
# SOURCE FILES
#--------------

set(SOURCES 
		  src/graphics.cpp  
		  src/simulation.cpp
		  src/agents.cpp
		  src/rnd_generators.cpp
		  src/main.cpp)

#---------
# TARGETS
#---------
add_executable(tragen ${SOURCES})
target_link_libraries(tragen ${LIBS})

if (O30-CELL_EXTRACTOR)
	add_executable(cellExtractor src/createListwithTextures.cpp)
	target_link_libraries(cellExtractor ${LIB_I3DALGO} ${LIB_I3DCORE})

	add_executable(cellRelabeler src/createLabeledMasks.cpp)
	target_link_libraries(cellRelabeler ${LIB_I3DALGO} ${LIB_I3DCORE})
endif (O30-CELL_EXTRACTOR)
