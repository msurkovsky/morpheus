add_library (MPIRelAnalysis SHARED
  MPIScopeAnalysis.cpp
  MPILabellingAnalysis.cpp
  )

target_include_directories (MPIRelAnalysis PRIVATE ${MORPHEUS_INCLUDES})
target_include_directories (MPIRelAnalysis SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
