
#include "llvm/IR/PassManager.h"

#include "morpheus/ADT/CommunicationNet.hpp"
#include "morpheus/ADT/CommNetFactory.hpp"
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"
#include "morpheus/Formats/DotGraph.hpp"
#include "morpheus/Transforms/GenerateMPNet.hpp"

using namespace llvm;

void interconnect_basicblock_cns(std::vector<cn::BasicBlockCN> &);

// -------------------------------------------------------------------------- //
// GenerateMPNetPass

PreservedAnalyses GenerateMPNetPass::run (Module &m, ModuleAnalysisManager &am) {

  am.registerPass([] { return MPILabellingAnalysis(); });
  am.registerPass([] { return MPIScopeAnalysis(); });

  MPIScope &mpi_scope = am.getResult<MPIScopeAnalysis>(m);
  MPILabelling &mpi_labelling = am.getResult<MPILabellingAnalysis>(m);

  Function *scope_fn = mpi_scope.getFunction();
  LoopInfo &loop_info = *mpi_scope.getLoopInfo();

  // create the CN representing scope function and following the CFG structure
  cn::CFG_CN cfg_cn(*scope_fn, loop_info);

  // for each basic block in CFG_CN add a pcn if possible
  for (cn::BasicBlockCN &bbcn : cfg_cn.bb_cns) {
    // plug-in nets for all MPI calls
    MPILabelling::MPICheckpoints checkpoints = mpi_labelling.get_mpi_checkpoints(bbcn.bb);
    while (!checkpoints.empty()) {
      auto checkpoint = checkpoints.front();
      if (checkpoint.second == MPICallType::DIRECT) { // TODO: first solve direct calls
        bbcn.add_pcn(cn::createCommSubnet(checkpoint.first));
        checkpoints.pop();
      }
    }
    // enclose the basic block cn
    bbcn.enclose();
  }

  // TODO: take rank/address value from the input code
  cn::AddressableCN acn(1);
  std::move(cfg_cn).inject_into(acn);
  // resolve unresolved elements
  acn.embedded_cn.resolve_unresolved();
  // enclose the cn
  acn.enclose();


  std::ofstream dot;
  dot.open("acn-" + acn.get_id() + ".dot");
  dot << acn;
  dot.close();

  acn.collapse();
  std::ofstream dot2;
  dot2.open("acn-" + acn.get_id() + "-collapsed.dot");
  dot2 << acn;
  dot2.close();

  return PreservedAnalyses::none(); // TODO: check which analyses have been broken?
}


