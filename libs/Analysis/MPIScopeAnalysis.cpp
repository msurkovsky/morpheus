
#include "CallFinder.hpp"
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/CallSite.h" // TODO: remove
#include "llvm/IR/Dominators.h" // TODO: remove
#include "llvm/Analysis/PostDominators.h" // TODO: remove
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <string>
#include <utility>
#include <map>

using namespace llvm;
using namespace std;

// NOTE: Morpheus -> mor, mrp, mrh ... posible prefixes

// The analysis runs over a module and tries to find a function that covers
// the communication, either directly (MPI_Init/Finalize calls) or mediately
// (via functions that calls MPI_Init/Finalize within)
MPIScopeAnalysis::Result
MPIScopeAnalysis::run(Module &m, ModuleAnalysisManager &am) {

  string main_f_name = "main";

  // Find the main function
  Function *main_fn = nullptr;
  for (auto &f : m) {
    if (f.hasName() && f.getName() == "main") {
      main_fn = &f;
      break;
    }
  }

  if (!main_fn) {
    return MPIScopeResult(); // empty (invalid) scope result
  }

  CallGraph cg(m);

  cg.print(errs());

  errs() << "========\n";

  FunctionAnalysisManager fam;

  MPILabellingAnalysis la;
  LabellingResult lr = la.run(*main_fn, fam);
  CallInst *ci = lr.get_unique_call("MPI_Init");

  CallGraphNode *cgn = cg[ci->getFunction()];

  errs() << "MAIN:\n";
  cgn->print(errs());
  for (CallGraphNode::CallRecord &cr : *cgn) {
    errs() << "ABC:\n";
    cr.second->print(errs());
  }


  /*

  if (lr.is_sequential(main_fn)) {
    return MPIScopeResult();
  }

  errs() << "CI: " << *ci << "\n";
  ImmutableCallSite ics(ci);
  errs() << "Caller: " << *ics.getCaller() << "\n";
  */
  /*
  Function *scope_candiate;
  if (lr.does_invoke_call(main_fn, "MPI_Init")) {
    scope_candiate = main_fn;
  } else {
    // indirect parallel calls
    const std::vector<CallInst *> &indirect_calls = lr.get_indirect_mpi_calls(main_fn);
    for (CallInst *inst : indirect_calls) {
      Function *f = inst->getCalledFunction();
      if (lr.does_invoke_call(f, "MPI_Init")) {
        scope_candiate = f;
        break;
      } else {
        // store  for further investigation
      }
    }
  }
  */


  return MPIScopeResult();
  /*
  string init_f_name = "MPI_Init";
  string finalize_f_name = "MPI_Finalize";


  // ... it should be enough to have a parent for each call

  // pair<CallInst *, CallInst *> caller_callee; // NOTE: to have an info who and where the callee was called

  CallInst *mpi_init, *mpi_finalize; // The exact calls of MPI_Init/Finalize calls
  CallInst *init_f, *finalize_f;

  do {
    for (auto &f : m) {
      init_f = find_call_in_by_name(init_f_name, f);
      finalize_f = find_call_in_by_name(finalize_f_name, f);

      // Store mpi initi/finalize if found
      // NOTE: the first appearance, i.e., null mpi_init,
      //       means that init_f represents MPI_Init call.
      if (!mpi_init && init_f) {
        mpi_init = init_f;
      }

      if (!mpi_finalize && finalize_f) {
        mpi_finalize = finalize_f;
      }

      // Looking for calls within one function that define the mpi scope
      if (init_f && finalize_f) {
        return Result(&f, init_f, finalize_f, mpi_init, mpi_finalize);
      }

      // If neither init_f nor finalize_f happened,
      // then either 'init_f' or 'finalize_f' might happened.
      if (init_f) {
        StringRef f_name = f.getName();
        assert(!f_name.empty()); // NOTE: called function has to have a name (actually it is not true but for now)
        init_f_name = f_name;
      } else if (finalize_f) {
        StringRef f_name = f.getName();
        assert(!f_name.empty());
        finalize_f_name = f_name;
      }
    } // end for
  } while(init_f || finalize_f); // continue search if at least one of the init/finalize call has been found.

  return Result(); // return empty scope; MPI is not involved at all within the module
  */
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;
