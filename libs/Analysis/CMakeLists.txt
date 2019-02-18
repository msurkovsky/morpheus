add_library (RankInfoAnalysis MODULE
  RankLocalizator.cpp
  tagrank_pass.cpp
  )

target_include_directories (RankInfoAnalysis PRIVATE ${MORPHEUS_INCLUDES})
target_include_directories (RankInfoAnalysis SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${USED_LLVM_INCLUDES}
  )
