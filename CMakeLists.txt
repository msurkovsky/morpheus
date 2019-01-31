cmake_minimum_required (VERSION 2.8)

# set (USED_LLVM_PATH "/home/sur096/install/llvm7.0.0")
set (USED_LLVM_PATH "/mnt/data/sur096/llvm")
set (USED_LLVM_INCLUDES "${USED_LLVM_PATH}/sources/llvm/include")
set (CMAKE_CXX_COMPILER "${USED_LLVM_PATH}/build/bin/clang++")

set(PROJECT_NAME "Morpheus")
project (${PROJECT_NAME})

set (LLVM_DIR ${USED_LLVM_PATH}/build/cmake/modules/CMakeFiles)
find_package (LLVM REQUIRED CONFIG)
message ("LLVM fount at ${LLVM_DIR}")

add_definitions (${LLVM_DEFINITIONS})

message("LLVM incls dirs: ${LLVM_INCLUDE_DIRS}")

# set (CMAKE_CXX_STANDARD 14)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs-bin)

# TODO: it will be clang related
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

add_subdirectory (libs)
add_subdirectory (src)

# target_link_libraries (${PROJECT_NAME}TagRankPass)
