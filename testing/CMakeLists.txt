cmake_minimum_required(VERSION 3.20)

project(testing)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")

find_library(LIB_I3DCORE "i3dcore")
find_library(LIB_I3DALGO "i3dalgo")
set(LIBS ${LIBS} ${LIB_I3DALGO} ${LIB_I3DCORE})

# GSL library
find_package(GSL REQUIRED)
set(LIBS ${LIBS} GSL::gsl GSL::gslcblas)

# Google rpc
find_package(gRPC CONFIG REQUIRED)
set(LIBS ${LIBS} gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)

# FMT
find_package(fmt CONFIG REQUIRED)
set(LIBS ${LIBS} fmt::fmt)

add_executable(main main.cpp)
target_link_libraries(main ${LIBS})
set_target_properties(main PROPERTIES CXX_STANDARD 20)
