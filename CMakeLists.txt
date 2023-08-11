cmake_minimum_required(VERSION 3.19)
project(arbor-sonata LANGUAGES CXX)
ENABLE_LANGUAGE(C)

cmake_policy(SET CMP0104 NEW)
set (CMAKE_CXX_STANDARD 17)

set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(arbor REQUIRED)
find_package(MPI REQUIRED CXX)
find_package(HDF5 REQUIRED)

add_library(sonata-private-deps INTERFACE)
target_link_libraries(sonata-private-deps INTERFACE)
install(TARGETS sonata-private-deps EXPORT sonata-targets)

add_library(sonata-public-deps INTERFACE)
install(TARGETS sonata-public-deps EXPORT sonata-targets)

add_subdirectory(sonata)
add_subdirectory(test)
add_subdirectory(arbata)
