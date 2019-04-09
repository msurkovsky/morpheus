
#include "CallFinder.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <string>
#include <utility>
#include <map>

using namespace llvm;
using namespace std;

// NOTE: Morpheus -> mor, mrp, mrh ... posible prefixes

namespace {
  template<typename IRUnitT>
  CallInst* find_call_in_by_name(const string &name, IRUnitT &unit) {
    auto filter = [name](const CallInst &inst) {
      if (auto *called_fn = inst.getCalledFunction()) {
        if (called_fn->getName() == name) {
          return true;
        }
      }
      return false;
    };
    vector<CallInst *> found_calls = CallFinder<IRUnitT>::find_in(unit, filter);
    // it is used to find MPI_Init and MPI_Finalize calls,
    // these cannot be called more than once.
    assert (found_calls.size() <= 1);
    if (found_calls.size() > 0) {
      return found_calls.front();
    }
    return nullptr;
  }

  // TODO: new interface should look like: (4.3.19: does it?)
  /*
    MPIInit_Call find_mpi_init(IRUnitT &unit); // this function can internally call "find_call_in_by_name" ...
   */

  using ExploreStates = DenseMap<const Value *, ExplorationState>;

  ExplorationState explore_bb(const BasicBlock *, ExploreStates &);

  ExplorationState explore_function(const Function *f, ExploreStates &current_states) {
    auto it = current_states.find(f);
    if (it != current_states.end()) {
      return it->getSecond();
    }

    if (f->hasName() && f->getName().startswith("MPI_")) {
      current_states.insert({ f, ExplorationState::MPI_CALL });
      return ExplorationState::MPI_CALL;
    }

    // NOTE: store the info that the function is currently being processed.
    //       This helps to resolve recursive calls.

    // => TEST: make a test to simple function calling itself. => it has to end with sequential
    current_states.insert({ f, ExplorationState::PROCESSING });

    ExplorationState res_es = ExplorationState::PROCESSING;
    for (const BasicBlock &bb : *f) {
      const ExplorationState& es = explore_bb(&bb, current_states);
      // NOTE: basic block cannot be directly of MPI_CALL type

      if (res_es < es) {
        res_es = es;
      }

      // NOTE: continue inspecting other functions to cover all calls.
    }

    if (res_es == ExplorationState::PROCESSING) { // At this point PROCESSING state
                                               // changes to SEQUENTIAL
      res_es = ExplorationState::SEQUENTIAL;
    }

    current_states[f] = res_es; // set the resulting status
    return res_es;
  }

  ExplorationState explore_bb(const BasicBlock* bb, ExploreStates &current_states) {
    auto it = current_states.find(bb);
    if (it != current_states.end()) {
      return it->getSecond();
    }

    ExplorationState res_es = ExplorationState::SEQUENTIAL;
    std::vector<CallInst *> call_insts = CallFinder<BasicBlock>::find_in(*bb);
    for (CallInst *call_inst : call_insts) {
      Function *called_fn = call_inst->getCalledFunction();
      if (called_fn) {
        const ExplorationState &es = explore_function(called_fn, current_states);

        if (es == ExplorationState::MPI_CALL) {
          res_es = ExplorationState::MPI_INVOLVED;
        } else if (es > ExplorationState::MPI_CALL) {
          res_es = ExplorationState::MPI_INVOLVED_MEDIATELY;
        }
      } else {
        errs() << "no called function for: '" << *call_inst << "'\n";
      }
    }

    current_states.insert({ bb, res_es });
    return res_es;
  }
}

namespace llvm {
  raw_ostream & operator<< (raw_ostream &out, const ExplorationState &s) {
    out << "eX state =";
    switch(s) {
    case ExplorationState::PROCESSING:
      out << " PROCESSING";
      break;
    case ExplorationState::SEQUENTIAL:
      out << " SEQUENTIAL";
      break;
    case ExplorationState::MPI_CALL:
      out << " MPI_CALL";
      break;
    case ExplorationState::MPI_INVOLVED:
      out << " MPI_INVOLVED";
      break;
    case ExplorationState::MPI_INVOLVED_MEDIATELY:
      out << " MPI_INVOLVED_MEDIATELY";
      break;
    }
    out << "\n";
    return out;
  }
}

// The analysis runs over a module and tries to find a function that covers
// the communication, either directly (MPI_Init/Finalize calls) or mediately
// (via functions that calls MPI_Initi/Finalize within)
MPIScopeAnalysis::Result
MPIScopeAnalysis::run(Module &m, ModuleAnalysisManager &am) {

  string main_f_name = "main";

  // Find the main function
  Function *main_unit = nullptr;
  for (auto &f : m) {
    if (f.hasName() && f.getName() == "main") {
      main_unit = &f;
      break;
    }
  }

  if (!main_unit) {
    return MPIScopeResult(); // empty (invalid) scope result
  }

  ExploreStates ess;
  ExplorationState es = explore_function(main_unit, ess);
  errs() << "Main function: " << es << "\n";
  for (auto it = ess.begin(); it != ess.end(); ++it) {
    errs() << it->getFirst()->getName() << ": " << it->getSecond() << "\n";
  }

  // for (auto &bb : *main_unit) {
  //   errs() << bb << "\n";
  // }

  PostDominatorTree pdt(*main_unit);
  // pdt.viewGraph();
  // auto *root_node = pdt.getRootNode();
  // errs() << "root node: " << *root_node->getBlock() << "\n"; // TODO: don't forget to investigate root node as well.

  // TODO: use post-dominator tree. Hence investigate the blocks in bottom-up order


  // for (BasicBlock &bb : *main_unit) {
  //   errs() << bb << "\n";
  // }

  // NOTE: traverse Dominator Tree
  auto *root_node = pdt.getRootNode();
  for (auto it = root_node->begin(); it != root_node->end(); it++) {
    auto *block = (*it)->getBlock();
    errs() << "bb: " << *block << "\n";
  }

  errs() << "\nPOST DOM TREE\n";
  for (auto node = GraphTraits<PostDominatorTree*>::nodes_begin(&pdt);
       node != GraphTraits<PostDominatorTree*>::nodes_end(&pdt);
       ++node) {

    // BasicBlock *bb = node->getBlock();
    errs() << "block:" << *node << "\n";
  }

  DominatorTree dt(*main_unit);
  errs() << "\nDOM TREE\n";
  for (auto node= GraphTraits<DominatorTree*>::nodes_begin(&dt);
       node != GraphTraits<DominatorTree*>::nodes_end(&dt);
       ++node) {
    errs() << "block:" << *node << "\n";
  }

  // for (auto it = root_node->begin(); it != root_node->end(); it++) {
  //   auto *block = (*it)->getBlock();
  //   errs() << "block: " << *block << "\n";
  //   // for (auto &instr : *block) {
  //   //     errs() << "\t" << instr << "\n";
  //   // }
  // }

  // ValueMap<Function*, bool> fstate;
  // ValueMap<Function*, bool> FNodesInTree;

  // FnExplorState fstate;
  // explore_function(main_unit, fstate);

  // ValueMap<const Function *, int> fState;
  // explore_function(main_unit, fState);

  // vector<CallInst*> main = CallFinder<Function>::find_in(*main_unit);
  // errs() << "Num of calls: " << main.size() << "\n";
  // int i = 1;
  // for (CallInst *inst : main) {
  //   if (auto *called_fn = inst->getCalledFunction()) {
  //     errs() << i << ": " << called_fn->getName() << "\n";
  //   }
  //   i++;
  // }

  return MPIScopeResult();

  /*
  string init_f_name = "MPI_Init";
  string finalize_f_name = "MPI_Finalize";


  // ... it should be enough to have a parent for each call

  // pair<CallInst *, CallInst *> caller_callee; // NOTE: to have an info who and where the callee was called

  CallInst *mpi_init, *mpi_finalize; // The exact calls of MPI_Init/Finalize calls
  CallInst *init_f, *finalize_f;

  do {
    for (auto &f : m) {
      init_f = find_call_in_by_name(init_f_name, f);
      finalize_f = find_call_in_by_name(finalize_f_name, f);

      // Store mpi initi/finalize if found
      // NOTE: the first appearance, i.e., null mpi_init,
      //       means that init_f represents MPI_Init call.
      if (!mpi_init && init_f) {
        mpi_init = init_f;
      }

      if (!mpi_finalize && finalize_f) {
        mpi_finalize = finalize_f;
      }

      // Looking for calls within one function that define the mpi scope
      if (init_f && finalize_f) {
        return Result(&f, init_f, finalize_f, mpi_init, mpi_finalize);
      }

      // If neither init_f nor finalize_f happened,
      // then either 'init_f' or 'finalize_f' might happened.
      if (init_f) {
        StringRef f_name = f.getName();
        assert(!f_name.empty()); // NOTE: called function has to have a name (actually it is not true but for now)
        init_f_name = f_name;
      } else if (finalize_f) {
        StringRef f_name = f.getName();
        assert(!f_name.empty());
        finalize_f_name = f_name;
      }
    } // end for
  } while(init_f || finalize_f); // continue search if at least one of the init/finalize call has been found.

  return Result(); // return empty scope; MPI is not involved at all within the module
  */
}

// provide definition of the analysis Key
AnalysisKey MPIScopeAnalysis::Key;
