
#include "morpheus/Analysis/RankLocalizator.hpp"
#include "morpheus/Analysis/IsRank.hpp"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

RankInfo RankAnalysis::run(Module &m, ModuleAnalysisManager &am) {

  errs() << "Run RankInfo Analysis\n";

  // auto &FAMProxy = am.getResult<FunctionAnalysisManagerModuleProxy>(m);
  // auto &FAM = FAMProxy.getManager();
  // FAM.registerPass([]() { return IsRank(); }); // yes this is working solution

  CallGetRankVisitor rv;
  rv.visit(m);

  for (auto *inst: rv.mpi_rank_insts) {
    errs() << "Calling rank function\n";
    errs() << *inst << "\n";
    // process the MPI call instruction
  }

  // RankInfo ri;
  // for (auto &f : m) {
  //   auto &R = FAM.template getResult<IsRank>(f);
  //   // errs() << "--->>> getting result of IsRank\n";
  // }

  // not working code
  // IsRank isRank;
  // for (const auto &f : m) {
  //   auto &r = isRank.run(f, FAM);
  // }
  return RankInfo();
}

AnalysisKey RankAnalysis::Key;
