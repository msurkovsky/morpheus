
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"

#include "morpheus/Transforms/MPIPruneProcess.hpp"

using namespace llvm;

// The possibility to call pass via opt
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK

llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "Morph", "v0.1", // TODO: expose version
      [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef PassName, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
          if (PassName == "pruneprocess") {
            // NOTE: the standard passes are already registered
            //       so I just add them, if needed.
            MPM.addPass(RequireAnalysisPass<CallGraphAnalysis, Module>());
            MPM.addPass(RequireAnalysisPass<ModuleSummaryIndexAnalysis, Module>());
            MPM.addPass(MPIPruneProcessPass());
          }
          return true;
        }
      );
    }
  };
}
