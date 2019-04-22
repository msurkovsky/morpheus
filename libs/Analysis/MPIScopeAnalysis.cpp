
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

  for (const auto &node : *cg) {
    CallGraphNode const *cgn = node.second.get();
    if (cgn->getFunction()) {
      if (cgn->getFunction()->getName() == "main") { // TODO: remove problem when some functions are explored and their "parent" is nullptr already -> TODO: implement less then on CallsTrack comparison to say when override marked function

        errs() << "FF: " << cgn->getFunction()->getName() << "\n";
        for (const CallGraphNode::CallRecord &cr : *cgn) {
          if (cr.second->getFunction()) { // process only non-external nodes
            CallsTrack ct = CallNode::create();
            process_call_record(cr, ct);
          }
        }

      }
    }
  }

  Instruction *inst = labelling->get_unique_call("MPI_Finalize");
  errs() << *inst << "\n";

  auto it = instructions_call_track.find(inst);
  if (it != instructions_call_track.end()) {
    CallsTrack ct = it->getSecond();
    errs() << *ct->parent->data << "\n";
  }
}

// private members ---------------------------------------------------------- //

MPIScope::CallsTrack
MPIScope::process_call_record(const CallGraphNode::CallRecord &cr, const CallsTrack &track) {
  Instruction *inst = CallSite(cr.first).getInstruction();
  auto it = instructions_call_track.find(inst);
  if (it != instructions_call_track.end()) {
    return it->getSecond();
  }

  CallsTrack res_track = CallNode::create(inst);
  res_track->parent = track;
  instructions_call_track[inst] = res_track; // TODO: reason this storage (prevent recursive calls but is it ok?)

  CallGraphNode *called_cgn = cr.second;
  for (CallGraphNode::CallRecord const &cr : *called_cgn) {
    if (cr.second->getFunction()) { // process only non-external nodes
      errs() << "Last: " << cr.second->getFunction()->getName() << "\n";
      process_call_record(cr, res_track);
    }
  }

  return res_track;
}
