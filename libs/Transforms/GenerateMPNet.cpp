
#include "llvm/IR/PassManager.h"

#include "morpheus/ADT/CommunicationNet.hpp"
#include "morpheus/ADT/CommNetFactory.hpp"
#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"
#include "morpheus/Formats/DotGraph.hpp"
#include "morpheus/Transforms/GenerateMPNet.hpp"

#include <iostream>

using namespace llvm;

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
      if (checkpoint.second == MPICallType::DIRECT) {
        bbcn.add_pcn(cn::createCommSubnet(checkpoint.first));
      } else {
        // TODO: implement reaction on other types of checkpoints
      }
      checkpoints.pop();
    }
    // enclose the basic block cn
    bbcn.enclose();
  }

  ConstantAsMetadata *rank_md = dyn_cast_or_null<ConstantAsMetadata>(
    m.getModuleFlag("morpheus.pruned_rank"));

  std::unique_ptr<cn::AddressableCN> acn;
  if (rank_md == nullptr) {
    acn = std::make_unique<cn::AddressableCN>("global");
  } else {
    ConstantInt *rank_v = dyn_cast<ConstantInt>(rank_md->getValue());
    auto rank = rank_v->getValue().getLimitedValue();
    acn = std::make_unique<cn::AddressableCN>("rank=" + std::to_string(rank));
  }

  std::move(cfg_cn).inject_into(*acn);
  // resolve unresolved elements
  acn->embedded_cn.resolve_unresolved();
  // enclose the cn
  acn->enclose();

  acn->collapse();

  string name = m.getSourceFileName();
  unsigned slash_pos = name.find_last_of("/");
  unsigned dot_pos = name.find_last_of(".");
  unsigned count = dot_pos - slash_pos - 1;
  if (dot_pos < name.size() && count > 0) {
    name = name.substr(slash_pos + 1, count);
  }

  std::cout << name + "-" + acn->address + ".dot" << std::endl;
  std::cout << *acn;

  return PreservedAnalyses::all();
}


