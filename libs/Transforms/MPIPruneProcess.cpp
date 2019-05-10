
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Support/CommandLine.h"

#include "morpheus/Analysis/MPILabellingAnalysis.hpp"
#include "morpheus/Analysis/MPIScopeAnalysis.hpp"
#include "morpheus/Transforms/MPIPruneProcess.hpp"

using namespace llvm;

// -------------------------------------------------------------------------- //
// MPIPruneProcessPass

static cl::opt<unsigned> rank_arg(
    "rank", cl::Required, cl::Hidden,
    cl::desc("An unsigned integer specifying rank of interest."));

PreservedAnalyses MPIPruneProcessPass::run (Module &m, ModuleAnalysisManager &am) {

  am.registerPass([] { return MPILabellingAnalysis(); });

  MPILabelling &mpi_labelling = am.getResult<MPILabellingAnalysis>(m);

  Instruction *comm_rank = mpi_labelling.get_unique_call("MPI_Comm_rank");
  errs() << *comm_rank << "\n";

  if (comm_rank) {
    CallSite cs_comm_rank(comm_rank);
    Value *rank_val = cs_comm_rank.getArgument(1);
    errs() << *rank_val << "\n";
    errs() << "Rank type: " << *rank_val->getType() << "\n";

    LLVMContext &ctx = comm_rank->getFunction()->getContext();
    IntegerType *t = IntegerType::get(ctx, 32);
    IntegerType *t2 = TypeBuilder<types::i<32>, false>::get(ctx);

    errs() << *t << " - " << *t2 << "\n";

    IRBuilder<> builder(comm_rank);
    ConstantInt *const_rank = builder.getInt32(rank_arg);
    AllocaInst *const_rank_alloc = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "rank");
    builder.CreateStore(const_rank, const_rank_alloc);

    errs() << "const_rank: " << *const_rank << "\n";

    errs() << "Users:\n";
    for (auto *user : rank_val->users()) {
      errs() << *user << "\n";
    }

    // rank_val->replaceAllUsesWith(const_rank_alloc); // NOTE: cannot use this, because it changes also MPI_Comm_rank call
    for (auto *user : rank_val->users()) { // .. instead of replaceAllUses I'm using call on the user
      if (user != comm_rank) {
        user->replaceUsesOfWith(rank_val, const_rank_alloc);
      }
    }

    errs() << "users of original rank:\n";
    for (auto *user : rank_val->users()) {
      errs() << *user << "\n";
    }
    errs() << "users of const rank:\n";
    for (auto *user : const_rank_alloc->users()) {
      errs() << *user << "\n";
    }
    // errs() << "Uses:\n";
    // for (auto &use : rank_val->uses()) {
    //   errs() << *use.getUser() << "\n";
    // }

    // Function *fn_comm_rank = cs_comm_rank.getCalledFunction();
    // if (fn_comm_rank->hasFnAttribute("rank")) {
    //   errs() << "YES\n";
    // } else {
    //   errs() << "NO\n";
    // }
  }

  // pruning code causes that all analyses are invalidated
  return PreservedAnalyses::none();
}
