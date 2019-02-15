
add_library (TagRankPass MODULE
  tagrank_pass.cpp
  testing_pass.cpp
  )

target_include_directories (TagRankPass SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${USED_LLVM_INCLUDES})
