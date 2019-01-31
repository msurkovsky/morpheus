
add_library (TagRankPass MODULE
  tagrank_pass.cpp
  )

target_include_directories (TagRankPass PRIVATE ${LLVM_INCLUDE_DIRS} ${USED_LLVM_INCLUDES})
