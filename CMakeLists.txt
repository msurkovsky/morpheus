cmake_minimum_required (VERSION 2.8)

set(PROJECT_NAME "Morpheus")
project (${PROJECT_NAME})

find_package (LLVM CONFIG REQUIRED)
add_definitions (${LLVM_DEFINITIONS})

message ("LLVM fount at ${LLVM_DIR}")

include_directories (${LLVM_INCLUDE_DIRS})

set (CMAKE_CXX_COMPILER "${LLVM_TOOLS_BINARY_DIR}/clang++")
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs-bin)

# TODO: it will be clang related
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

add_subdirectory (libs)
add_subdirectory (src)

# target_link_libraries (${PROJECT_NAME}TagRankPass)
