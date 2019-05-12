
#include "llvm/Support/CommandLine.h"

#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"
#include "morpheus/Transforms/MPIPruneProcess.hpp"

using namespace llvm;

// -------------------------------------------------------------------------- //
// MPIPruneProcessPass

// NOTE: no the problem remains the same, but why?
static cl::opt<unsigned> targ("mujarg",
                              cl::init(7),
                              cl::desc("Some description of the testing argument"));

PreservedAnalyses MPIPruneProcessPass::run (Module &m, ModuleAnalysisManager &am) {
  errs() << "TAG RANK: before\n";

  // All of my passes needs to be registered before used.
  am.registerPass([] { return MPILabellingAnalysis(); });
  am.registerPass([] { return MPIScopeAnalysis(); });

  MPIScope &mpi_scope = am.getResult<MPIScopeAnalysis>(m);
  MPILabelling &mpi_labelling = am.getResult<MPILabellingAnalysis>(m);

  errs() << "targ: " << targ << "\n";
  // TODO: use the result of analyses -> rename this to SplitByRank pass

  errs() << "TAG RANK: after\n";

  return PreservedAnalyses::all();
}
