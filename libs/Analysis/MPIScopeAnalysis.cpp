
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
  raw_ostream &operator<< (raw_ostream &out, const MPIScope::CallNodeDataT &data) {
    out << data.second->getName();
    return out;
  }
}

using namespace llvm;

// -------------------------------------------------------------------------- //
// MPIScopeAnalysis

// The analysis runs over a module and tries to find a function that covers
// the communication, either directly (MPI_Init/Finalize calls) or indirectly
// (via functions that calls MPI_Init/Finalize inside)
MPIScopeAnalysis::Result
MPIScopeAnalysis::run(Module &m, ModuleAnalysisManager &mam) {

  ModuleSummaryIndex &index = mam.getResult<ModuleSummaryIndexAnalysis>(m);
  MPILabelling &labelling = mam.getResult<MPILabellingAnalysis>(m);
  CallGraph &cg = mam.getResult<CallGraphAnalysis>(m);

  return MPIScope(index, labelling, cg);
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;

// -------------------------------------------------------------------------- //
// MPIScope

MPIScope::MPIScope(ModuleSummaryIndex &index, MPILabelling &labelling, CallGraph &cg)
    : index(index),
      cg(cg),
      labelling(labelling) {

  Instruction *mpi_init_call = labelling.get_unique_call("MPI_Init");
  Instruction *mpi_fin_call = labelling.get_unique_call("MPI_Finalize");
  if (!mpi_init_call || !mpi_fin_call) {
    // errs() << "There is no MPI area defined by MPI_Init & MPI_Finalize calls.\n";
    scope_fn = nullptr;
    return;
  }

  // remove old instruction-calls tracks
  instruction_calls_track.clear();

  // use the index to calculate root functions (those which are not called)
  FunctionSummary root_nodes = index.calculateCallGraphRoot();
  ArrayRef<FunctionSummary::EdgeTy> edges = root_nodes.calls();

  // filter the root nodes from the call graph and count number of call instructions
  unsigned int call_inst_count = 0;
  std::vector<CallGraphNode *> cg_roots;
  cg_roots.reserve(edges.size());

  // filter root nodes from call graph
  for (const std::pair<ValueInfo, CalleeInfo> &edge : edges) {
    StringRef fn_name = std::get<ValueInfo>(edge).name();
    for (const auto &node : cg) {
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

  // process root nodes
  for (CallGraphNode *cgn : cg_roots) {
    Function *fn = cgn->getFunction();
    VisitedNodes vn;
    vn.reserve(cgn->size());
    process_cgnode(cgn, CallNode::create({std::nullopt, fn}), vn);
  }

  /*
  // Testing print
  for (auto &p : instruction_calls_track) {
    CallsTrack &ct = p.second;
    errs() << *p.first << "(" << p.first << ") - [" << *ct << "]\n";
  }
  */

  // calculate the scope function
  // NOTE: current implementation counts on singe call of MPI_Init/Finalize.
  CallsTrack &ct_init = instruction_calls_track[mpi_init_call];
  CallsTrack &ct_finalize = instruction_calls_track[mpi_fin_call];

  CallsTrack deeper, shallower;
  if (ct_init->get_depth() > ct_finalize->get_depth()) {
    deeper = ct_init;
    shallower = ct_finalize;
  } else {
    deeper = ct_finalize;
    shallower = ct_init;
  }

  // start with a direct caller of MPI_Init/Finalize
  CallsTrack dp = deeper->get_parent();
  CallsTrack sp = shallower->get_parent();

  // equalize the depth levels
  while (dp->get_depth() > sp->get_depth()) {
    dp = dp->get_parent();
  }

  // continue up to the common predecessor, if there is any
  while(dp != sp || dp->get_depth() > 0) {
    dp = dp->get_parent();
    sp = sp->get_parent();
  }

  if (dp == sp) { // MPI Scope
    scope_fn = dp->data.second;
    loop_info = LoopInfo(DominatorTree(*scope_fn));
  } else { // NO Scope
    scope_fn = nullptr;
  }
}

Function *MPIScope::getFunction() {
  return scope_fn;
}

LoopInfo *MPIScope::getLoopInfo() {
  return &loop_info;
}

bool MPIScope::isValid() {
  return scope_fn != nullptr;
}

// private members ---------------------------------------------------------- //

void MPIScope::process_cgnode(CallGraphNode const *cgn, const CallsTrack &track, VisitedNodes &visited) {

  if (visited.count(cgn) == 1) {
    return;
  }

  visited[cgn] = true;

  for (const CallGraphNode::CallRecord &cr : *cgn) {
    Function *fn = cr.second->getFunction();
    if (fn) { // process only non-external nodes
      Instruction *inst = CallSite(cr.first).getInstruction();
      CallsTrack ct = CallNode::create({inst, fn});
      ct->set_parent(track);

      instruction_calls_track.insert({inst, ct});
      process_cgnode(cr.second, ct, visited);
    }
  }
}
