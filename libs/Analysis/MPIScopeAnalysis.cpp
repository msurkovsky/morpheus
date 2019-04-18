
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/CallSite.h" // TODO: remove
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <string>

using namespace llvm;

// NOTE: Morpheus -> mor, mrp, mrh ... posible prefixes

// The analysis runs over a module and tries to find a function that covers
// the communication, either directly (MPI_Init/Finalize calls) or mediately
// (via functions that calls MPI_Init/Finalize within)
MPIScopeAnalysis::Result
MPIScopeAnalysis::run(Module &m, ModuleAnalysisManager &mam) {

  // Find the main function
  Function *main_fn = nullptr;
  for (auto &f : m) {
    if (f.hasName() && f.getName() == "main") {
      main_fn = &f;
      break;
    }
  }

  assert(main_fn && "No main function within the module.");

  // TODO: this does not work!! -- causes run-time error
  // FunctionAnalysisManager &fam = mam.getResult<FunctionAnalysisManagerModuleProxy>(m).getManager();
  // fam.registerPass([]() { return MPILabellingAnalysis(); });
  // auto res = fam.getCachedResult<MPILabellingAnalysis>(*main_fn);

  // MPILabellingAnalysis la;
  MPILabelling res(*main_fn); // This does work

  Instruction *i = res.get_unique_call("MPI_Finalize");
  errs() << *i << "\n";


  return MPIScopeResult();
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;
