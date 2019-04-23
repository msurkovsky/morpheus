
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/IR/CallSite.h" // TODO: remove
#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
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

  ModuleSummaryIndex &index = mam.getResult<ModuleSummaryIndexAnalysis>(m);

  // MPILabelling labelling(cg);
  // Instruction *i = labelling.get_unique_call("MPI_Finalize");
  // errs() << *i << "\n";

  return MPIScope(index, cg);
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;

// -------------------------------------------------------------------------- //
// MPIScope

MPIScope::MPIScope(ModuleSummaryIndex &index, std::shared_ptr<CallGraph> &cg)
    : index(index),
      cg(cg),
      labelling(std::make_unique<MPILabelling>(cg)) {

  FunctionSummary cg_root_nodes = index.calculateCallGraphRoot();

  ArrayRef<FunctionSummary::EdgeTy> edges = cg_root_nodes.calls();

  // auto it = cg->begin();
  // do {
  //   it = std::search(it, cg->end(),
  //                    edges.begin(), edges.end(),
  //                    [](const auto &node, const auto &edge) {
  //                      Function *f = node.second->getFunction();
  //                      if (!f) {
  //                        return false;
  //                      }
  //                      // errs() << "f: " << f->getName() << "; name: " << edge.first.name() << "\n";
  //                      return f->getName() == edge.first.name();
  //                    });

  //   if (it != cg->end()) {
  //     errs() << "FF: " << it->second->getFunction()->getName() << "\n";
  //   }
  //   it++;
  // } while (it == cg->end());

  for (const std::pair<ValueInfo, CalleeInfo> &edge : edges) {
    StringRef fn_name = std::get<ValueInfo>(edge).name();
    for (const auto &node : *cg) {
      Function *fn = node.second->getFunction();
      if (fn && fn_name == fn->getName()) { // process only root nodes
        errs() << "Processing node: " << fn_name << "\n";
        process_call_record(node.second, CallNode::create({std::nullopt, fn}));
      }
    }
  }
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
MPIScope::process_call_record(const std::unique_ptr<CallGraphNode> &cgn, const CallsTrack &track) {

  return CallNode::create(track->data);
  // return CallNode::create();

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
