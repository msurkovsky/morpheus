add_subdirectory (ADT)
add_subdirectory (Analysis)
add_subdirectory (Transforms)

add_library(Utils SHARED
  Utils.cpp
  )

target_include_directories (Utils PRIVATE ${MORPHEUS_INCLUDES})
target_include_directories (Utils SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

add_library(Morph MODULE
  morph.cpp
  )

target_include_directories (Morph PRIVATE ${MORPHEUS_INCLUDES})
target_include_directories (Morph SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

target_link_libraries (Morph Utils MorphADT MPIRelAnalysis MPIRelTransforms)
