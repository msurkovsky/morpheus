add_library(MPIRelTransforms SHARED
  MPISubstituteRank.cpp
  GenerateMPNet.cpp
  )

target_include_directories (MPIRelTransforms PRIVATE ${MORPHEUS_INCLUDES})
target_include_directories (MPIRelTransforms SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${USED_LLVM_INCLUDES}
  )

# target_link_libraries (MPIRelTransforms MPIRelAnalysis)
# target_link_libraries (MPIRelTransforms LLVMSupport)
