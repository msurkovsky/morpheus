
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/CallSite.h" // TODO: remove
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <string>

using namespace llvm;

// -------------------------------------------------------------------------- //
// MPIScopeAnalysis

// The analysis runs over a module and tries to find a function that covers
// the communication, either directly (MPI_Init/Finalize calls) or mediately
// (via functions that calls MPI_Init/Finalize within)
MPIScopeAnalysis::Result
MPIScopeAnalysis::run(Module &m, ModuleAnalysisManager &mam) {

  // TODO: this does not work!! -- causes run-time error
  // FunctionAnalysisManager &fam = mam.getResult<FunctionAnalysisManagerModuleProxy>(m).getManager();
  // fam.registerPass([]() { return MPILabellingAnalysis(); });
  // auto res = fam.getCachedResult<MPILabellingAnalysis>(*main_fn);

  // MPILabellingAnalysis la;

  std::shared_ptr<CallGraph> cg = std::make_shared<CallGraph>(m);

  // MPILabelling labelling(cg);
  // Instruction *i = labelling.get_unique_call("MPI_Finalize");
  // errs() << *i << "\n";

  return MPIScope(cg);
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;

// -------------------------------------------------------------------------- //
// MPIScope

MPIScope::MPIScope(std::shared_ptr<CallGraph> &cg)
    : cg(cg),
      labelling(std::make_unique<MPILabelling>(cg)) {

  // TODO: call explore cg
}

// private members ---------------------------------------------------------- //

MPIScope::CallsTrack
MPIScope::process_call_record(CallGraphNode::CallRecord const &cr, const CallsTrack &track) {

  CallSite call_site(cr.first);
  CallsTrack res_track = CallNode::create(call_site);
  res_track->parent = track;

  CallGraphNode *called_cgn = cr.second;
  for (CallGraphNode::CallRecord const &cr : *called_cgn) {
    process_call_record(cr, res_track);
  }
  return res_track;
}
