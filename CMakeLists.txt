cmake_minimum_required (VERSION 2.8)

set (LLVM_ROOT_PATH $ENV{LLVM_ROOT_PATH})
if (NOT DEFINED LLVM_ROOT_PATH)
  message (FATAL_ERROR "Varialbe LLVM_ROOT_PATH is not set.")
  return ()
endif ()
message (STATUS "LLVM_ROOT_PATH: ${LLVM_ROOT_PATH}")

set (USED_LLVM_PATH $ENV{LLVM_ROOT_PATH})
# set (USED_LLVM_PATH "/home/sur096/install/llvm7.0.0")
set (USED_LLVM_INCLUDES "${USED_LLVM_PATH}/sources/include")
set (CMAKE_CXX_COMPILER "${USED_LLVM_PATH}/build/bin/clang++")

set(PROJECT_NAME "Morpheus")
project (${PROJECT_NAME})

set (LLVM_DIR ${USED_LLVM_PATH}/build/cmake/modules/CMakeFiles)
find_package (LLVM REQUIRED CONFIG)
message ("LLVM fount at ${LLVM_DIR}")

add_definitions (${LLVM_DEFINITIONS})

message("LLVM include dirs: ${LLVM_INCLUDE_DIRS}")

# set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs-bin)

# TODO: it will be clang related
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

add_subdirectory (libs)
add_subdirectory (src)

# target_link_libraries (${PROJECT_NAME}TagRankPass)
