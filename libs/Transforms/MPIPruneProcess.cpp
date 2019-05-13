
#include "llvm/Support/CommandLine.h"

#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"
#include "morpheus/Transforms/MPIPruneProcess.hpp"

using namespace llvm;

// -------------------------------------------------------------------------- //
// MPIPruneProcessPass

static cl::opt<unsigned> rank_arg(
    "rank", cl::Required, cl::Hidden,
    cl::desc("An unsigned integer specifying rank of interest."));

PreservedAnalyses MPIPruneProcessPass::run (Module &m, ModuleAnalysisManager &am) {

  am.registerPass([] { return MPILabellingAnalysis(); });

  MPILabelling &mpi_labelling = am.getResult<MPILabellingAnalysis>(m);

  Instruction *comm_rank = mpi_labelling.get_unique_call("MPI_Comm_rank");
  errs() << *comm_rank << "\n";

  if (comm_rank) {
    CallSite cs_comm_rank(comm_rank);
    Value *rank_val = cs_comm_rank.getArgument(1);
    errs() << *rank_val << "\n";
    errs() << "Users:\n";
    for (auto *user : rank_val->users()) {
      errs() << *user << "\n";
    }
    errs() << "Uses:\n";
    for (auto &use : rank_val->uses()) {
      errs() << *use << "\n";
    }

    // Function *fn_comm_rank = cs_comm_rank.getCalledFunction();
    // if (fn_comm_rank->hasFnAttribute("rank")) {
    //   errs() << "YES\n";
    // } else {
    //   errs() << "NO\n";
    // }
  }

  // pruning code causes that all analyses are invalidated
  return PreservedAnalyses::none();
}
