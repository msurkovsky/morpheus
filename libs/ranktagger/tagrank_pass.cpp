
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "testing_pass.hpp"

using namespace llvm;

namespace {
  struct TagRankPass : public PassInfoMixin<TagRankPass> {
    PreservedAnalyses run (Module &M, ModuleAnalysisManager &MAM) {
      // errs() << "Tag Rank Pass runs on module.\n";
      // errs() << "\t Module name: " << M.getName() << "\n";

      // errs() << "\t Instrs count: " << M.getInstructionCount() << "\n";

      // -----------------------------------------------------------------------

      // NOTE: how to make a pipeline of analysis on the same level
      // FunctionPassManager fpm;
      // fpm.addPass(TestingFnPass());
      // fpm.run(Function &IR, AnalysisManager<Function> &AM)

      // -----------------------------------------------------------------------

      /* >>> NOTE: How to do it  without inner analysis manager proxy?
      <<< */

      /* >>> NOTE: The same as ModuleFunctionPassAdaptor below; however under my control

      using MY_FunctionAnalysisManagerModuleProxy = InnerAnalysisManagerProxy<FunctionAnalysisManager, Module>;

      FunctionAnalysisManager &fam = MAM.getResult<MY_FunctionAnalysisManagerModuleProxy>(M).getManager();

      TestingFnPass tfmPass;
      for(auto &f : M) {
        tfmPass.run(f, fam);
      }
      <<< */

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      errs() << "TAG RANK: before\n";

      ModuleToFunctionPassAdaptor<TestingFnPass> adaptor = createModuleToFunctionPassAdaptor(TestingFnPass());
      adaptor.run(M, MAM);

      errs() << "TAG RANK: after\n";

      // -----------------------------------------------------------------------
      /*
      for (auto& f : M.functions()) {
        if (f.hasName()) {
          // errs() << "Function: " << f.getName() << "\n";
          if (f.getName() == "main") {
            for (auto &bb : f.getBasicBlockList()) {
              // errs() << " BB: " << bb.getName() << "\n";
              for (Instruction &inst : bb.getInstList()) {
                if (inst.getOpcode() == Instruction::Call) {
                  auto &ci = cast<CallInst>(inst);
                  const Function *called_fn = ci.getCalledFunction();
                  if (called_fn->getName() == "MPI_Comm_rank") {
                    errs() << "Called FUNCTION: " << *called_fn << "\n";
                    errs() << "I'm calling Comm rank\n";
                    errs() << "instr: " << ci << "\n";
                    errs() << "#arg operands: " << ci.getNumArgOperands() << "\n";
                    auto *v = ci.getArgOperand(1); // get rank??
                    errs() << "V: " << *v << "\n";

                    for (auto *u : v->users()) {
                      errs() << "user: " << *u << "\n";
                    }
                    // errs() << "V: " << *v << "\n";
                    // for (auto *u : v->users()) {
                    //   errs() << "u: " << *u << "\n";
                    //   for (auto &op : u->operands()) {
                    //     errs() << "u-Op: " << *op << "\n";
                    //   }
                    //   errs() << "\n";
                    //   // if (!inst.isIdenticalTo(u->get())) {
                    //   //   errs() << "user of V: " << *u << "\n";
                    //   // }
                    // }
                    // for (auto *u : v->users()) {
                    //   for (auto &op : u->operands()) {
                    //     errs() << "oop: " << op << "\n";
                    //   }
                    // }
                    // errs() << "# uses: " << v->getNumUses() << "\n";
                    // errs() << "# operands: " << v- << "\n";
                    // for (auto &attr : ci.getAttributes()) {
                    //   errs() << "Attr: " << attr.getAsString() << "\n";
                    // }
                  }
                }
              }
            }
          }
        }
      }
      */

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
