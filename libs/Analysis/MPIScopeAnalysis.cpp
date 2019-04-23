
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/IR/CallSite.h" // TODO: remove
#include "llvm/IR/ModuleSummaryIndex.h"
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


  ModuleSummaryIndex &msi = mam.getResult<ModuleSummaryIndexAnalysis>(m);
  FunctionSummary fs = msi.calculateCallGraphRoot();

  auto calls = fs.calls();
  errs() << "# calls: " << calls.size() << "\n";
  for (auto call : calls) {
    errs() << "Call: " << call.first.name() << "\n";
    // << call.first.getRef()->second.getOriginalName() << "\n";
    // auto lst = call.first.getSummaryList();
    // for (auto &l : lst) {
    //   errs() << l->getOriginalName() << "\n";
    //   // errs() << *l->getBaseObject() << "\n";
    // }
    errs() << "\n";
  }

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

  cg->print(errs());
  /*
  for (const auto &node : *cg) {
    CallGraphNode const *cgn = node.second.get();
    if (cgn->getFunction()) {
        errs() << "FF: " << cgn->getFunction()->getName() << "\n";
        CallsTrack ct = CallNode::create();
        process_call_record(cgn, ct);
    }
  }

  Instruction *inst = labelling->get_unique_call("MPI_Finalize");
  errs() << *inst << "\n";

  auto it = instructions_call_track.find(inst);
  if (it != instructions_call_track.end()) {
    CallsTrack ct = it->getSecond();
    errs() << ct->metadata <<  " -> " << ct->parent->metadata << "\n";
  }
  */
}

// private members ---------------------------------------------------------- //

MPIScope::CallsTrack
MPIScope::process_call_record(CallGraphNode const *cgn, const CallsTrack &track) {



  return CallNode::create();

  /*
  Instruction *inst = CallSite(cr.first).getInstruction();
  auto it = instructions_call_track.find(inst);
  if (it != instructions_call_track.end()) {
    return it->getSecond();
  }

  CallsTrack res_track = CallNode::create(inst);
  res_track->parent = track;
  res_track->metadata = CallSite(inst).getCalledFunction()->getName();
  instructions_call_track[inst] = res_track; // TODO: reason this storage (prevent recursive calls but is it ok?)

  CallGraphNode *called_cgn = cr.second;
  for (CallGraphNode::CallRecord const &inner_cr : *called_cgn) {
    if (inner_cr.second->getFunction()) { // process only non-external nodes
      errs() << "Last: " << inner_cr.second->getFunction()->getName() << "\n";
      process_call_record(inner_cr, res_track);
    }
  }

  return res_track;
  */
}
