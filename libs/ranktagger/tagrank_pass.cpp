
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h" // TODO: do I need this?

using namespace llvm;

namespace {
  struct TagRankPass : public PassInfoMixin<TagRankPass> {
    PreservedAnalyses run (Module &M, ModuleAnalysisManager &MAM) {
      // errs() << "Tag Rank Pass runs on module.\n";
      // errs() << "\t Module name: " << M.getName() << "\n";

      // errs() << "\t Instrs count: " << M.getInstructionCount() << "\n";

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
