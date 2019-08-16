add_subdirectory (ADT)
add_subdirectory (Analysis)
add_subdirectory (Transforms)

add_library(Morph MODULE
  morph.cpp
  )

target_include_directories (Morph PRIVATE ${MORPHEUS_INCLUDES})
target_include_directories (Morph SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

target_link_libraries (Morph MorphADT MPIRelAnalysis MPIRelTransforms)
