
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

using namespace llvm;

namespace {
  struct TagRankPass : public PassInfoMixin<TagRankPass> {
    PreservedAnalyses run (Module &m, ModuleAnalysisManager &am) {

      errs() << "TAG RANK: before\n";


      // // Register TestingAnalysisPass to the function analysis manager
      // FunctionAnalysisManager &fam = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
      // pass builder for testing analysis pass
      auto pb_tap = [](){ return MPIScopeAnalysis(); };
      am.registerPass(pb_tap);

      // Run the module adaptor
      // adaptor.run(M, MAM);

      // ModuleToFunctionPassAdaptor<RankAnalysis> adaptor = createModuleToFunctionPassAdaptor(RankAnalysis());
      // for (auto &f : M) {
      //   auto &res = fam.getResult<RankAnalysis>(f);
      // }
      auto &res = am.getResult<MPIScopeAnalysis>(m);
      for (auto &inst : res) {
        errs() << "INST: " << inst.getValueID() << "\n";
        // if (&inst) {
        //   errs() << "INST: " << inst.getValueID() << "\n";
        //   errs() << "\t >>> " << inst << "\n";
        // } else {
        //   errs() << "AAA\n";
        // }
      }

      // auto it = res.begin();
      // errs() << "IT: " << *it << "\n";
      // errs() << "Is scope valid: " << (res.isValid() ? "YES" : "NO") << "\n";
      // errs() << "Scope: " << res.scope->getName() << "\n";
      // errs() << "begin: " << res.start->getCalledFunction()->getName() << "\n";
      // errs() << "end: " << res.end->getCalledFunction()->getName() << "\n";


      errs() << "TAG RANK: after\n";

      return PreservedAnalyses::all();
    }
  };
} // end of anonymous namespace


// The possibility to call pass via opt
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK

llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "TagRankPass", "v0.1", // TODO: expose version
      [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef PassName, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
          if (PassName == "tag-rank-pass") {
            MPM.addPass(TagRankPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}
