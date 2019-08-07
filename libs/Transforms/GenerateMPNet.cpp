
#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/IR/PassManager.h"

#include "morpheus/ADT/CommunicationNet.hpp"
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"
#include "morpheus/Transforms/GenerateMPNet.hpp"

using namespace llvm;

// -------------------------------------------------------------------------- //
// GenerateMPNetPass

PreservedAnalyses GenerateMPNetPass::run (Module &m, ModuleAnalysisManager &am) {

  am.registerPass([] { return MPILabellingAnalysis(); });
  am.registerPass([] { return MPIScopeAnalysis(); });

  MPIScope &mpi_scope = am.getResult<MPIScopeAnalysis>(m);
  MPILabelling &mpi_labelling = am.getResult<MPILabellingAnalysis>(m);

  Function *scope_fn = mpi_scope.getFunction();

  CommunicationNet cn;
  using CN = CommunicationNet;

  auto bfs_it = breadth_first(scope_fn);
  for (const BasicBlock *bb : bfs_it) {
    MPILabelling::MPICheckpoints checkpoints = mpi_labelling.get_mpi_checkpoints(bb);
    errs() << bb << ": \n";
    while (!checkpoints.empty()) {
      auto checkpoint = checkpoints.front();
      if (checkpoint.second == MPICallType::DIRECT) { // TODO: first solve direct calls
        errs() << "\t" << *checkpoint.first << "\n"; // instruction
        checkpoints.pop();
      }
    }
  }
  cn.print();

  return PreservedAnalyses::none(); // TODO: check which analyses have been broken?
}
