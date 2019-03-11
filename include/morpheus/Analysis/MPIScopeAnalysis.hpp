
//===----------------------------------------------------------------------===//
//
// Description of the file ... basic idea to provide a scope of elements
// that are involved in MPI either directly or indirectly
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/PassManager.h"
#include "llvm/IR/SymbolTableListTraits.h"

#include <vector>
#include <string>
#include <iterator>

namespace llvm {

  class ScopeIterator
    : public std::iterator<std::forward_iterator_tag, Instruction> { // TODO: the iterator has to be self contained
    // Note: I "cannot use" defined iterators, just because I'm going throw
    // more Basicblocks. In fact, I'm writing something like flat iterator.

    Instruction *begin_inst;
    Instruction *end_inst;
    SymbolTableList<Instruction>::iterator current;

  public:
    ScopeIterator() : begin_inst(nullptr), end_inst(nullptr) { }

    explicit ScopeIterator(Instruction *begin, Instruction *end)
      : current(begin->getIterator()), begin_inst(begin), end_inst(end) { }

    ScopeIterator(const ScopeIterator &sci) = default;

    ScopeIterator& operator=(const ScopeIterator& sci) = default;

    ScopeIterator begin() {
      if (begin_inst) {
        current = begin_inst->getIterator();
      }
      return *this;
    }

    ScopeIterator end() {
      if (end_inst) {
        current = end_inst->getIterator();
      }
      return *this;
    }

    ScopeIterator& operator++() {
      current = std::next(current);
      auto *terminator = current->getParent()->getTerminator();
      if (&*current == terminator) {
        auto *p = current->getParent();
        auto *node = p->getNextNode();
        errs() << "Parent: " << p << "\n";
        errs() << "Node: " << node << "\n";

        if (!p || !node) { // TODO: is it possible to have nullptr parent?
          errs() << ">>>error<<< " << p << " -- " << node << "\n";
          ScopeIterator end_it = end();
          return end_it;
        }
        current = node->begin();
        errs() << "Curent: " << *current << "\n";
        // errs() << "Parrent: " << *current << "\n";
        // ScopeIterator end_it = end();
        // return end_it;
        // then go up and continue from there.
        errs() << "\n";
      }
      return *this;
    }

    ScopeIterator operator++(int) {
      ScopeIterator tmp = *this;
      ++*this;
      return tmp;
    }

    Instruction& operator*() const {
      return *current;
    }

    Instruction* operator->() const {
      return &operator*();
    }

    bool operator==(const ScopeIterator &other) const {
      return (begin_inst == other.begin_inst &&
              end_inst == other.end_inst &&
              current == other.current);
    }

    bool operator!=(const ScopeIterator &other) const {
      return !operator==(other);
    }
  }; // end of ScopeIterator


  class MPIScopeAnalysis;

  // TODO: does it make sense to implement ilist_node_with_parent (as same as basic block?)
  class MPIScopeResult { // TODO: make the data private

    Function *scope; // NOTE: it has to be a function ("worst case" is main)
    CallInst *start_f; // call that invokes MPI_Init within 'scope'
    CallInst *end_f;   // call that invokes MPI_Finalize within 'scope'

    CallInst *init_call;     // call of MPI_Init
    CallInst *finalize_call; // call of MPI_Finalize
    ScopeIterator iter;

  public:
    using iterator = ScopeIterator;

    MPIScopeResult() : scope(nullptr), start_f(nullptr), end_f(nullptr),
                       init_call(nullptr), finalize_call(nullptr) { };

    MPIScopeResult(Function *scope, CallInst *start_f, CallInst *end_f,
                   CallInst* init_call, CallInst *finalize_call)
      : scope(scope), start_f(start_f), end_f(end_f),
        init_call(init_call), finalize_call(finalize_call),
        iter(ScopeIterator(init_call, finalize_call)) { }

    MPIScopeResult(const MPIScopeResult &sci) = default;

    ScopeIterator begin() { return iter.begin(); }
    ScopeIterator end()   { return iter.end(); }

    bool isValid() {
      return scope != nullptr;
      // Instruction *test_i = init_call->next();
      // auto node = init_call->getNextNode();
      errs() << "\n\tInit call instr: " << *init_call << "\n";

      errs() << "\tisa<Inst>: " << isa<Instruction>(init_call) << "\n";
      // ---------------->>>
      auto next = init_call->getNextNode(); // TODO: It seems that getNextNode is the right instruction I'm looking for!!
      errs() << "\tNext: " << *next << "\n";
      // ----------------<<<

      auto *bb = init_call->getParent();
      // const auto &list = bb->*(BasicBlock::getSublistAccess((Instruction *) nullptr));
      // const auto &list = bb->*(BasicBlock::getSublistAccess(init_call));


      // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
      auto it2 =  init_call->getIterator(); // this is the solution I want to use!
      // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
      // NOTE: the `end` is either nullptr or finalize_call


      // auto next2 = list.getNextNode(init_call);
      // errs() << "\tNxtC: " << *next2 << "\n";
      auto it = bb->begin();
      errs() << "\tIterator: " << *it << "\n";
      errs() << "\tIt2:     " << *it2 << "\n";
      auto next2 = std::next(it2);
      errs() << "\tIt next2: " << *next2 << "\n";
      // auto &it = init_call->iterator;
      return scope != nullptr;
    }
    // NOTE:
    // Well, in the end I need a kind of navigation trough the scope. Therefore, a set of queries needs to be defined.

    // TODO: define it as an iterator over "unfolded" scope
  };


  class MPIScopeAnalysis : public AnalysisInfoMixin<MPIScopeAnalysis> { // both -*-module-*- and function analysis (->4.3.19: I don't think so. *module* is enough)
    static AnalysisKey Key;
    friend AnalysisInfoMixin<MPIScopeAnalysis>;

  public:

    using Result = MPIScopeResult;

    MPIScopeResult run (Module &, ModuleAnalysisManager &);
  };
}
