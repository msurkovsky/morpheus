#include "CallFinder.hpp"

#include "morpheus/Analysis/MPILabellingAnalysis.hpp"

#include <cassert>


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

// TODO: for both of the follwoing ctor copy/move the inner structres

MPILabelling::MPILabelling(const MPILabelling &labelling)
    : root_fn(labelling.root_fn) {

  cg = std::make_unique<CallGraph>(CallGraph(*labelling.root_fn.getParent()));

  // TODO: copy the built structure
}

// TODO: maybe a default will be OK - CallGraph defines it
MPILabelling::MPILabelling(MPILabelling &&labelling)
    : root_fn(labelling.root_fn), cg(std::move(labelling.cg)) { }

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

  CallGraphNode *cgn = (*cg)[f];

  ExplorationState res_es = SEQUENTIAL;
  // for (instr, inner_cgn) in cgn:
  for (const CallGraphNode::CallRecord &cr : *cgn) {
    ExplorationState inner_es = SEQUENTIAL;

    CallSite call_site(cr.first);
    Function *called_fn = call_site.getCalledFunction();

    const ExplorationState &es = explore_function(called_fn);
    switch(es) {
    case MPI_CALL:
      mpi_calls[called_fn->getName()].push_back(call_site);
      inner_es = MPI_INVOLVED;
      mpi_affected_bblocks[call_site->getParent()].emplace_back(call_site, MPICallType::DIRECT);
      break;
    case MPI_INVOLVED:
    case MPI_INVOLVED_MEDIATELY:
      inner_es = MPI_INVOLVED_MEDIATELY;
      mpi_affected_bblocks[call_site->getParent()].emplace_back(call_site, MPICallType::INDIRECT);
    }

    if (res_es < inner_es) {
      res_es = inner_es;
    }
  }

  /* // NOTE: original version using basic blocks
  ExplorationState res_es = SEQUENTIAL;
  for (const BasicBlock &bb : *f) {
    const ExplorationState& es = explore_bb(&bb);
    // NOTE: basic block cannot be directly of MPI_CALL type

    if (res_es < es) {
      res_es = es;
    }

    // NOTE: continue inspecting other functions to cover all calls.
  }
  */

  fn_labels[f] = res_es; // set the resulting status
  return res_es;
}

void MPILabelling::explore_inst(Instruction const *inst, Instruction const *caller) {
}

MPILabelling::ExplorationState
MPILabelling::explore_bb(const BasicBlock *bb) {

  ExplorationState res_es = SEQUENTIAL;

  /*
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
  */

  return res_es;
}

Instruction *MPILabelling::get_unique_call(StringRef name) const {

  auto search = mpi_calls.find(name);
  if (search == mpi_calls.end()) {
    return nullptr;
  }

  const std::vector<CallSite> &calls = search->second;
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

bool MPILabelling::does_invoke_call(
    Function const *f,
    StringRef name
) const {

  /*
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
  */
  return false;
}

std::vector<CallInst *>
MPILabelling::get_indirect_mpi_calls(Function const *f) const {

  std::vector<CallInst *> indirect_calls;
  /*
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
  */

  return indirect_calls;
}
