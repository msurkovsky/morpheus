
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"

using namespace llvm;


namespace {

  struct TagRankPass : public PassInfoMixin<TagRankPass> {

    PreservedAnalyses run (Module &m, ModuleAnalysisManager &am) {
      errs() << "TAG RANK: before\n";

      // All of my passes needs to be registered before used.
      am.registerPass([] { return MPILabellingAnalysis(); });
      am.registerPass([] { return MPIScopeAnalysis(); });

      MPIScope &mpi_scope = am.getResult<MPIScopeAnalysis>(m);
      MPILabelling &mpi_labelling = am.getResult<MPILabellingAnalysis>(m);

      // TODO: use the result of analyses -> rename this to SplitByRank pass

      errs() << "TAG RANK: after\n";

      return PreservedAnalyses::all();
    }
  };
} // end of anonymous namespace


// The possibility to call pass via opt

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK

llvmGetPassPluginInfo() {
  return { // creates and return PassPluginInfo object
    LLVM_PLUGIN_API_VERSION, "TagRankPass", "v0.1", // TODO: expose version
      [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef PassName, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
          errs() << "PassName: " << PassName << "\n";
          if (PassName == "tagrank") {
            // NOTE: the standard passes are already registered
            //       so I just add them, if needed.
            MPM.addPass(RequireAnalysisPass<CallGraphAnalysis, Module>());
            MPM.addPass(RequireAnalysisPass<ModuleSummaryIndexAnalysis, Module>());
            MPM.addPass(TagRankPass());
          }
          return true;
          // return false; // TODO: that's make a mistake of disability to parse pass pipeline
        }
      );
    }
  };
}
