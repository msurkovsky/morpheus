
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"

#include <cassert>
#include <algorithm>

using namespace llvm;

// -------------------------------------------------------------------------- //
// MPILabellingAnalysis

MPILabelling
MPILabellingAnalysis::run (Function &f, FunctionAnalysisManager &fam) {
  return MPILabelling(f);
}

// provide definition of the analysis Key
AnalysisKey MPILabellingAnalysis::Key;


// -------------------------------------------------------------------------- //
// MPILabelling

MPILabelling::MPILabelling(Function &f)
    : root_fn(f) {

  cg = std::make_unique<CallGraph>(CallGraph(*f.getParent()));
  explore_function(&f);
}

MPILabelling::MPILabelling(MPILabelling const &labelling)
    : root_fn(labelling.root_fn),
      fn_labels(labelling.fn_labels),
      mpi_calls(labelling.mpi_calls),
      bb_mpi_checkpoints(labelling.bb_mpi_checkpoints) {

  // CG has to be build from scratch
  cg = std::make_unique<CallGraph>(CallGraph(*labelling.root_fn.getParent()));
}

// Public API --------------------------------------------------------------- //

Instruction *MPILabelling::get_unique_call(StringRef name) const {

  auto search = mpi_calls.find(name);
  if (search == mpi_calls.end()) {
    return nullptr;
  }

  std::vector<CallSite> const &calls = search->second;
  // TODO: isn't there any support of error messages in llvm infrastructure?
  assert(calls.size() == 1 && "Expect single call.");

  return calls[0].getInstruction();
}

bool MPILabelling::is_sequential(Function const *f) const {
  return check_status<SEQUENTIAL>(f);
}

bool MPILabelling::is_mpi_involved(Function const *f) const {
  return (check_status<MPI_INVOLVED>(f) ||
          check_status<MPI_INVOLVED_MEDIATELY>(f));
}

MPILabelling::MPICheckpoints
MPILabelling::get_mpi_checkpoints(BasicBlock const *bb) const {
  auto search = bb_mpi_checkpoints.find(bb);
  if (search == bb_mpi_checkpoints.end()) {
    return MPICheckpoints();
  }

  return search->second;
}

MPILabelling::ExplorationState
MPILabelling::explore_function(Function const *f) {
  auto it = fn_labels.find(f);
  if (it != fn_labels.end()) {
    return it->getSecond();
  }

  if (f->hasName() && f->getName().startswith("MPI_")) {
    fn_labels[f] = MPI_CALL;
    return MPI_CALL;
  }

  // NOTE: store the info that the function is currently being processed.
  //       This helps to resolve recursive calls as it immediately returns
  //       PROCESSING status.

  // TODO: => TEST: make a test to simple function calling itself. => it has to end with sequential
  fn_labels[f] = PROCESSING;

  ExplorationState res_es = SEQUENTIAL;

  CallGraphNode *cgn = (*cg)[f];
  for (CallGraphNode::CallRecord const &cr : *cgn) {
    ExplorationState inner_es = SEQUENTIAL;

    CallSite call_site(cr.first);
    Function *called_fn = call_site.getCalledFunction();

    ExplorationState const &es = explore_function(called_fn);
    switch(es) {
    case MPI_CALL:
      mpi_calls[called_fn->getName()].push_back(call_site);
      inner_es = MPI_INVOLVED;
      save_checkpoint(call_site.getInstruction(), MPICallType::DIRECT);
      break;
    case MPI_INVOLVED:
    case MPI_INVOLVED_MEDIATELY:
      inner_es = MPI_INVOLVED_MEDIATELY;
      save_checkpoint(call_site.getInstruction(), MPICallType::INDIRECT);
    }

    if (res_es < inner_es) {
      res_es = inner_es;
    }
  }

  fn_labels[f] = res_es; // set the resulting status
  return res_es;
}

void MPILabelling::save_checkpoint(Instruction *inst, MPICallType call_type) {
  assert(CallSite(inst) && "Only call site instruction can be saved.");

  BasicBlock *bb = inst->getParent();
  assert(bb != nullptr && "Null parent of instruction.");

  bb_mpi_checkpoints[bb].emplace(inst->getIterator(), call_type);
}


/*
bool MPILabelling::does_invoke_call( // TODO: this belongs to scope analysis
    Function const *f,
    StringRef name
) const {

  CallGraphNode *cgn = (*cg)[f];
  for (CallGraphNode::CallRecord const &cr : *cgn) {
    // auto it = mpi_affected_calls.find(&bb);
    // assert (it != mpi_affected_calls.end()); // TODO: is it correct?

    // const std::vector<std::pair<CallInst *, MPICallType>> &calls = it->second;
    // for (const std::pair<CallInst *, MPICallType> &call_type : calls) {
    //   CallInst * inst = call_type.first;
    //   StringRef inst_name = inst->getCalledFunction()->getName();
    //   if (call_type.second == MPICallType::DIRECT && inst_name == name) {
    //     return true;
    //   }
    // }
  }
  return false;
}
*/
