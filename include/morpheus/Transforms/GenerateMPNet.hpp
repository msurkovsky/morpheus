
//===----------------------------------------------------------------------===//
//
// GenerateMPNet
//
//===----------------------------------------------------------------------===//

#ifndef MRPH_GENERATE_MPN_H
#define MRPH_GENERATE_MPN_H

#include "llvm/IR/CFG.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

  struct GenerateMPNetPass : public PassInfoMixin<GenerateMPNetPass> {

    PreservedAnalyses run (Module &m, ModuleAnalysisManager &am);
  };
} // end llvm

#endif // MRPH_GENERATE_MPN_H
