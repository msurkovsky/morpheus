#include "CallFinder.hpp"

#include "llvm/Analysis/CallGraph.h"
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"

#include <cassert>


using namespace llvm;

// -------------------------------------------------------------------------- //
// MPILabellingAnalysis

LabellingResult
MPILabellingAnalysis::run (Function &f, FunctionAnalysisManager &fam) {

  CallGraph cg(*f.getParent());

  CallGraphNode *cgn_f  = cg[&f];

  errs() << "TESTING:\n";
  cgn_f->print(errs());
  LabellingResult result;
  result.explore_function(&f);

  return result;
}

// provide definition of the analysis Key
AnalysisKey MPILabellingAnalysis::Key;


// -------------------------------------------------------------------------- //
// LabellingResult

LabellingResult::ExplorationState
LabellingResult::explore_function(const Function *f) {
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
  for (const BasicBlock &bb : *f) {
    const ExplorationState& es = explore_bb(&bb);
    // NOTE: basic block cannot be directly of MPI_CALL type

    if (res_es < es) {
      res_es = es;
    }

    // NOTE: continue inspecting other functions to cover all calls.
  }

  fn_labels[f] = res_es; // set the resulting status
  return res_es;
}

LabellingResult::ExplorationState
LabellingResult::explore_bb(const BasicBlock *bb) {

  ExplorationState res_es = SEQUENTIAL;
  std::vector<CallInst *> call_insts = CallFinder<BasicBlock>::find_in(*bb);
  for (CallInst *call_inst : call_insts) {
    Function *called_fn = call_inst->getCalledFunction();

    assert(called_fn != nullptr);

    ExplorationState es = explore_function(called_fn);
    if (es == MPI_CALL) {
      res_es = MPI_INVOLVED;
      mpi_calls[called_fn->getName()].push_back(call_inst);
      mpi_affected_calls[bb].push_back({ call_inst, MPICallType::DIRECT });
    } else if (es > MPI_CALL) {
      res_es = ExplorationState::MPI_INVOLVED_MEDIATELY;
      mpi_affected_calls[bb].push_back({ call_inst, MPICallType::INDIRECT });
    }
  }

  return res_es;
}

CallInst * LabellingResult::get_unique_call(StringRef name) const {

  auto search = mpi_calls.find(name);
  if (search == mpi_calls.end()) {
    return nullptr;
  }

  const std::vector<CallInst *> &calls = search->second;
  assert(calls.size() == 1);

  return calls[0];
}

bool LabellingResult::is_sequential(Function const *f) const {
  return check_status<SEQUENTIAL>(f);
}

bool LabellingResult::is_mpi_involved(Function const *f) const {
  return (check_status<MPI_INVOLVED>(f) ||
          check_status<MPI_INVOLVED_MEDIATELY>(f));
}

bool LabellingResult::does_invoke_call(
    Function const *f,
    StringRef name
) const {

  for (const BasicBlock &bb : *f) {
    auto it = mpi_affected_calls.find(&bb);
    assert (it != mpi_affected_calls.end()); // TODO: is it correct?

    const std::vector<std::pair<CallInst *, MPICallType>> &calls = it->second;
    for (const std::pair<CallInst *, MPICallType> &call_type : calls) {
      CallInst * inst = call_type.first;
      StringRef inst_name = inst->getCalledFunction()->getName();
      if (call_type.second == MPICallType::DIRECT && inst_name == name) {
        return true;
      }
    }
  }
  return false;
}

std::vector<CallInst *>
LabellingResult::get_indirect_mpi_calls(Function const *f) const {

  std::vector<CallInst *> indirect_calls;
  for (const BasicBlock &bb : *f) {
    auto it = mpi_affected_calls.find(&bb);
    assert (it != mpi_affected_calls.end()); // TODO: is it correct?

    const std::vector<std::pair<CallInst *, MPICallType>> &calls = it->second;
    for (const std::pair<CallInst *, MPICallType> &call_type : calls) {
      if (call_type.second == MPICallType::INDIRECT) {
        indirect_calls.push_back(call_type.first);
      }
    }
  }

  return indirect_calls;
}
