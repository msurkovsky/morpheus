
#include "morpheus/Analysis/IsRank.hpp"

#include "llvm/Support/raw_ostream.h"


using namespace llvm;

IsRank::Result IsRank::run(Function &f, FunctionAnalysisManager &am) {

  CallGetRankVisitor rv;
  rv.visit(f);

  return IsRank::Result();
}

AnalysisKey IsRank::Key;
