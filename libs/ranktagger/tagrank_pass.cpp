
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h" // TODO: do I need this?

using namespace llvm;

namespace {
  struct TagRankPass : public PassInfoMixin<TagRankPass> {
    PreservedAnalyses run (Function &F, FunctionAnalysisManager &FAM) {
      if (F.hasName()) {
        errs() << "Tag Rank" << F.getName() << "\n";
      }
      return PreservedAnalyses::all();
    }
  };
} // end of anonymous namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "TagRankPass", "v0.1", // TODO: expose version
      [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef PassName, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
          if (PassName == "tag-rank-pass") {
            FPM.addPass(TagRankPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}
