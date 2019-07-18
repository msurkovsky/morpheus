
#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/IR/PassManager.h"

#include "morpheus/ADT/CommunicationNet.hpp"
#include "morpheus/ADT/CommNetFactory.hpp"
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


  // TODO: take rank/address value from the input code
  cn::AddressableCN acn(1);

  auto bfs_it = breadth_first(scope_fn);
  for (const BasicBlock *bb : bfs_it) {
    MPILabelling::MPICheckpoints checkpoints = mpi_labelling.get_mpi_checkpoints(bb);
    /* TODO: remove */ errs() << bb << ": \n";
    while (!checkpoints.empty()) {
      auto checkpoint = checkpoints.front();
      if (checkpoint.second == MPICallType::DIRECT) { // TODO: first solve direct calls

        cn::PluginCNGeneric pcn = cn::createCommSubnet(checkpoint.first);
        std::move(pcn).inject_into(acn); // acn takes over the pcn elements

        /* TODO: remove */ errs() << "\t" << *checkpoint.first.getInstruction() << "\n"; // instruction
        checkpoints.pop();
      }
    }
  }

  acn.print(errs());
  acn.collapse();
  acn.print(errs());

  return PreservedAnalyses::none(); // TODO: check which analyses have been broken?
}
