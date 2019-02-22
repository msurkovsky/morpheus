
#include "CallFinder.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/Support/raw_ostream.h"

#include <cassert>

using namespace llvm;
using namespace std;

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
}

// The analysis runs over a module and tries to find a function that covers
// the communication, either directly (MPI_Init/Finalize calls) or mediately
// (via functions that calls MPI_Initi/Finalize within)
MPIScopeAnalysis::Result
MPIScopeAnalysis::run(Module &m, ModuleAnalysisManager &am) {

  MPIScopeResult result; // based on partial results I should fill the overall one.

  StringRef init_f_name = "MPI_Init";
  StringRef finalize_f_name = "MPI_Finalize";

  CallInst *init_f, *finalize_f;
  while (true) {
    for (auto &f : m) {
      init_f = find_call_in_by_name(init_f_name, f);
      finalize_f = find_call_in_by_name(finalize_f_name, f);

      if (init_f && finalize_f) {
        result.scope = &f;
        result.start = init_f;
        result.end = finalize_f;
        break; // skip other search, but that's ok (because only one MPI_Init/Finalize can be presented)
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
    }

    if (init_f && finalize_f) {
      break;
    }

    if (!init_f && !finalize_f) {
      break;
    }
  }

  return result;
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;
