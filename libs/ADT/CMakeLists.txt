add_library(MorphADT SHARED
  CommunicationNet.cpp
  )

target_include_directories (MorphADT PRIVATE ${MORPHEUS_INCLUDES})
target_include_directories (MorphADT SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${USED_LLVM_INCLUDES}
  )
