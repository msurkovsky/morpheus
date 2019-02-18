
#include "morpheus/Analysis/RankLocalizator.hpp"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

RankInfo run(Module &M, ModuleAnalysisManager &AM) {

  errs() << "Run RankInfo Analysis\n";
  return RankInfo();
}

AnalysisKey RankAnalysis::Key;
