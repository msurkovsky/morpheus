
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "testing_pass.hpp"

using namespace llvm;

PreservedAnalyses TestingFnPass::run(Function &F, FunctionAnalysisManager &FAM) {
  errs() << "TFP fn: " << F.getName() << "\n";
  return PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK

llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "TestingFnPass", "v0.1",
      [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef PassName, FunctionPassManager &FPN, ArrayRef<PassBuilder::PipelineElement>) {
          if (PassName == "testing-fn-pass") {
            FPN.addPass(TestingFnPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}

