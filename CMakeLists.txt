cmake_minimum_required(VERSION 3.24)

project(gnss-tool
  VERSION 0.1.0
  DESCRIPTION "GNSS positioning toolkit: SPP / PPP / RTK"
  LANGUAGES CXX Fortran
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(FortranCInterop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/modules)

enable_testing()

add_subdirectory(src/core)
add_subdirectory(src/io)
add_subdirectory(src/bindings)
add_subdirectory(src/cli)
add_subdirectory(tests)
