
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

namespace llvm {
  raw_ostream &operator<< (raw_ostream &out, const MPIScope::InnerCallNode &inode) {
    out << inode.second->getName();
    return out;
  }
}

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

  // use the index to calculate root functions (those which are not called)
  FunctionSummary root_nodes = index.calculateCallGraphRoot();
  ArrayRef<FunctionSummary::EdgeTy> edges = root_nodes.calls();

  // filter the root nodes from the call graph and count number of call instructions
  unsigned int call_inst_count = 0;
  std::vector<CallGraphNode *> cg_roots;
  cg_roots.reserve(edges.size());

  for (const std::pair<ValueInfo, CalleeInfo> &edge : edges) {
    StringRef fn_name = std::get<ValueInfo>(edge).name();
    for (const auto &node : *cg) {
      CallGraphNode *cgn = node.second.get();
      Function *fn = cgn->getFunction();
      if (fn && fn_name == fn->getName()) {
        cg_roots.push_back(cgn); // store root node
        call_inst_count += cgn->size(); // add number of called functions from root node
      }
    }
  }

  // reserve the size of unordered map with instructions pointing to its call track
  instruction_calls_track.reserve(call_inst_count);

  for (CallGraphNode *cgn : cg_roots) {
    Function *fn = cgn->getFunction();
    errs() << "FN: " << fn->getName() << "\n";
    process_cgnode(cgn, CallNode::create({std::nullopt, fn})); // TODO: do I work with instruction inside?
  }

  // Testing print
  for (auto &p : instruction_calls_track) {
    CallsTrack &ct = p.second;
    errs() << *p.first << "(" << p.first << ") - [" << *ct << "]\n";
  }


  // calculating scope function (TODO:)
  Instruction *inst = labelling->get_unique_call("MPI_Finalize"); // TODO: should it be only one? For the purspose of simplicity YES!
  errs() << "\n" << *inst << "\n";

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

void MPIScope::process_cgnode(const CallGraphNode *cgn, const CallsTrack &track) {

  for (const CallGraphNode::CallRecord &cr : *cgn) {
    Function *fn = cr.second->getFunction();
    if (fn) { // process only non-external nodes
      Instruction *inst = CallSite(cr.first).getInstruction();
      CallsTrack ct = CallNode::create({inst, fn});
      ct->parent = track;

      instruction_calls_track.insert({inst, ct});
      process_cgnode(cr.second, ct);
    }
  }

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
