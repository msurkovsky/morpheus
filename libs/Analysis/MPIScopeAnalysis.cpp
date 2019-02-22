
#include "CallFinder.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <string>

using namespace llvm;
using namespace std;

// NOTE: Morpheus -> mor, mrp, mrh ... posible prefixes

namespace {
  template<typename IRUnitT>
  CallInst* find_call_in_by_name(const string &name, IRUnitT &unit) {
    vector<CallInst*> found_calls = CallFinder<IRUnitT>::find_in(name, unit);
    // it is used to find MPI_Init and MPI_Finalize calls,
    // these cannot be called more than once.
    assert (found_calls.size() <= 1);
    if (found_calls.size() > 0) {
      return found_calls.front();
    }
    return nullptr;
  }

  // TODO: new interface should look like:
  /*
    MPIInit_Call find_mpi_init(IRUnitT &unit); // this function can internally call "find_call_in_by_name" ...
   */
}

// The analysis runs over a module and tries to find a function that covers
// the communication, either directly (MPI_Init/Finalize calls) or mediately
// (via functions that calls MPI_Initi/Finalize within)
MPIScopeAnalysis::Result
MPIScopeAnalysis::run(Module &m, ModuleAnalysisManager &am) {

  MPIScopeResult result; // based on partial results I should fill the overall one.

  string init_f_name = "MPI_Init";
  string finalize_f_name = "MPI_Finalize";

  // ... it should be enough to have a parent for each call

  CallInst *mpi_init, *mpi_finalize;
  CallInst *init_f, *finalize_f;

  while (true) {
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
        result.scope = &f;
        result.start = init_f;
        result.end = finalize_f;
        result.init = mpi_init;
        result.finalize = mpi_finalize;
        break; // skip other search; it's ok (because only one MPI_Init/Finalize can be presented)
      }

      if (init_f) {
        StringRef f_name = f.getName();
        assert(!f_name.empty()); // called function has to have a name (actually it is not true but for now)
        init_f_name = f_name;
        continue; // finalize_f is definitely nullptr now, hence continue from here
      }

      if (finalize_f) {
        StringRef f_name = f.getName();
        assert(!f_name.empty());
        finalize_f_name = f_name;
      }
    } // end for

    if (init_f && finalize_f) {
      // I found the scope
      break;
    }

    // At this point I went through all the possible functions in module
    // and did not find valid scope.
    if (!init_f && !finalize_f) {
      // MPI is not involved at all
      break;
    }
  }

  return result;
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;
