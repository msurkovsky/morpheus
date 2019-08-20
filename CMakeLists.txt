cmake_minimum_required (VERSION 2.8)

set(PROJECT_NAME "Morpheus")
project (${PROJECT_NAME})

set(MORPHEUS_INCLUDES "${CMAKE_SOURCE_DIR}/include")

find_package (LLVM 8 REQUIRED CONFIG)

add_definitions (${LLVM_DEFINITIONS})

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fPIC -fno-rtti -g -O0 -std=c++17")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs-bin)

# TODO: it will be clang related
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

add_subdirectory (include/morpheus)

add_subdirectory (libs)
