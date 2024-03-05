/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define DEBUG_TYPE "frameloadstoreopts"

#include "hermes/IR/Analysis.h"
#include "hermes/IR/CFG.h"
#include "hermes/IR/IRBuilder.h"
#include "hermes/IR/Instrs.h"
#include "hermes/Optimizer/PassManager/Pass.h"
#include "hermes/Support/Statistic.h"

#include "llvh/ADT/SetOperations.h"
#include "llvh/Support/Debug.h"

/// This pass tries to deduplicate loads and delete unobservable stores to frame
/// variables.
///
/// For loads, the key idea is that if there are no instructions that may write
/// a variable between two loads or a store and a load to it, the second load
/// may be eliminated. This is accomplished by creating a mirror of the variable
/// on the stack and invalidating it when a call may store to it.
/// For stores, the idea is that if there are no instructions that may read a
/// variable between two stores, then the first store is redundant.
///
/// In both cases, the analysis is refined to allow instructions with
/// side-effects in the middle, by checking whether a variable has capturing
/// loads/stores that may manipulate it from such an instruction. For example,
/// this means that we can deduplicate loads across a function call, as long as
/// we know that there are no capturing stores.

namespace hermes {
namespace {

class CapturedVariables {
 public:
  llvh::DenseSet<Variable *> loads;
  llvh::DenseSet<Variable *> stores;
};

/// Take the intersection of the sets in \p values associated with the range of
/// blocks \p range.
template <typename RangeTy>
llvh::DenseSet<Variable *> blockIntersect(
    RangeTy &&range,
    const llvh::DenseMap<BasicBlock *, llvh::DenseSet<Variable *>> &values) {
  auto it = std::begin(range), e = std::end(range);
  if (it == e)
    return {};

  // Copy the first set as a starting point.
  auto res = values.lookup(*it++);

  // Remove any values that are not the same in all sets.
  for (; it != e; ++it) {
    auto valIt = values.find(*it);

    // If no entry exists for this block, bail.
    if (valIt == values.end())
      return {};

    llvh::set_intersect(res, valIt->second);
  }

  return res;
}

class FunctionLoadStoreOptimizer {
  /// The function being optimized.
  Function *const F_;

  /// Describes whether a variable has been captured anywhere in the program.
  const CapturedVariables &globalCV_;

  /// Map from each variable accessed in this function to the stack location
  /// created for it.
  llvh::DenseMap<Variable *, AllocStackInst *> variableAllocas_;

  /// Post order analysis for this function.
  std::vector<BasicBlock *> PO_;

  /// Map from a basic block to the set of variables that have valid values in
  /// their corresponding stack locations at the end of the block.
  llvh::DenseMap<BasicBlock *, llvh::DenseSet<Variable *>> blockValidVariables_;

  /// For each variable that is loaded from in F_, create an alloca that can
  /// be used to perform load elimination, and set it in \c variableAllocas_.
  void createVariableAllocas() {
    IRBuilder builder(F_);
    // Insert the alloca at the start of the function, after any FirstInBlock
    // instructions.
    auto insertAt = F_->begin()->begin();
    while (insertAt->getSideEffect().getFirstInBlock())
      ++insertAt;
    builder.setInsertionPoint(&*insertAt);

    for (BasicBlock *BB : PO_) {
      for (Instruction &I : *BB) {
        if (auto *LFI = llvh::dyn_cast<LoadFrameInst>(&I)) {
          Variable *V = LFI->getLoadVariable();
          // If there isn't already an alloca for V, create one and insert it
          // into the map.
          auto [it, first] = variableAllocas_.try_emplace(V, nullptr);
          if (first)
            it->second =
                builder.createAllocStackInst(V->getName(), V->getType());
        }
      }
    }
  }

  /// Delete any allocas in \p variableAllocas_ that ended up not being used to
  /// eliminate loads. That is, delete any allocas that are only used by stores.
  void deleteUnusedAllocas() {
    IRBuilder::InstructionDestroyer destroyer;
    for (auto [V, ASI] : variableAllocas_) {
      // Skip this instruction if there are any non-store users.
      if (!llvh::all_of(ASI->getUsers(), llvh::isa<StoreStackInst, Value *>))
        continue;

      for (auto *u : ASI->getUsers())
        destroyer.add(u);
      destroyer.add(ASI);
    }
  }

  /// Attempts to replace loads from the frame in \p BB with loads from stack
  /// locations that have been populated by a previous load or store to the same
  /// variable. Uses capture information from globalCV_ to determine whether
  /// intervening operations may store to the variable and prevent forwarding.
  bool eliminateLoads(BasicBlock *BB) {
    // Compute the set of variables that currently have valid values in their
    // corresponding stack location. Loads from these variables may be
    // eliminated. The stack location is only valid if it is valid in all
    // predecessors. Note that this implementation is conservative, since some
    // predecessors may not yet have been visited. For the best results, we
    // should traverse the blocks in RPO order.
    auto validVariables =
        blockIntersect(predecessors(BB), blockValidVariables_);

    IRBuilder builder(BB->getParent());
    IRBuilder::InstructionDestroyer destroyer;

    bool changed = false;

    for (Instruction &I : *BB) {
      if (auto *SF = llvh::dyn_cast<StoreFrameInst>(&I)) {
        // If a stack location exists for this variable, store to it and
        // insert the variable into the set of valid values.
        auto it = variableAllocas_.find(SF->getVariable());
        if (it != variableAllocas_.end()) {
          builder.setInsertionPoint(SF);
          builder.createStoreStackInst(SF->getValue(), it->second);
          validVariables.insert(SF->getVariable());
        }
        continue;
      }

      // Try to replace the LoadFrame with a recently saved value.
      if (auto *LF = llvh::dyn_cast<LoadFrameInst>(&I)) {
        // Check if we already have a valid value for the load in its
        // corresponding stack location. If so, use it, otherwise, populate it.
        builder.setInsertionPointAfter(LF);
        auto *V = LF->getLoadVariable();
        auto *alloca = variableAllocas_.lookup(LF->getLoadVariable());
        if (validVariables.insert(V).second) {
          // No entry currently exists, store the result of this load to the
          // stack.
          builder.createStoreStackInst(LF, alloca);
        } else {
          // We already have a known value for this variable. Replace the load
          // with it.
          LF->replaceAllUsesWith(builder.createLoadStackInst(alloca));
          destroyer.add(LF);
        }

        changed = true;
        continue;
      }

      // Invalidate the variable storage if the instruction may execute
      // capturing stores that write the variable.
      if (I.getSideEffect().getExecuteJS()) {
        for (auto it = validVariables.begin(); it != validVariables.end();
             ++it) {
          // If there are any captured stores of the variable, the value may be
          // updated, so the stack location is no longer valid.
          if (globalCV_.stores.count(*it))
            validVariables.erase(it);
        }
      }
    }

    // Store the valid variables for this block so subsequent blocks can use it.
    auto [it, first] =
        blockValidVariables_.try_emplace(BB, std::move(validVariables));
    (void)first;
    assert(first && "Block already visited");

    return changed;
  }

  /// Attempts to remove redundant stores to the frame in \p BB when we can
  /// determine they cannot be observed.
  bool eliminateStores(BasicBlock *BB) {
    // Map from a Variable to the last store to it that we have not found to be
    // observable yet. If an entry exists when a subsequent store is
    // encountered, the entry's store is not observable and may be eliminated.
    llvh::DenseMap<Variable *, StoreFrameInst *> prevStores;

    // Deletes instructions when we leave the function.
    IRBuilder::InstructionDestroyer destroyer;

    bool changed = false;

    for (Instruction &I : *BB) {
      // Try to delete the previous store based on the current store.
      if (auto *SF = llvh::dyn_cast<StoreFrameInst>(&I)) {
        auto [it, inserted] = prevStores.try_emplace(SF->getVariable(), SF);

        if (!inserted) {
          // There is a previous store, delete it and make this the previous
          // store.
          destroyer.add(it->second);
          changed = true;
          it->second = SF;
        }
        continue;
      }

      // If we are reading from the variable, the last known store cannot be
      // eliminated.
      if (auto *LF = llvh::dyn_cast<LoadFrameInst>(&I)) {
        prevStores.erase(LF->getLoadVariable());
        continue;
      }

      // Invalidate the store frame storage if the instruction may execute
      // capturing loads that observe this store, or throw an exception that
      // allows prior stores to be observed.
      auto sideEffect = I.getSideEffect();
      if (sideEffect.getExecuteJS() || sideEffect.getThrow())
        prevStores.clear();
    }
    return changed;
  }

 public:
  FunctionLoadStoreOptimizer(Function *F, const CapturedVariables &globalCV)
      : F_(F), globalCV_(globalCV) {
    PO_ = postOrderAnalysis(F);
  }

  bool run() {
    // Create an alloca for each variable we want to optimize.
    createVariableAllocas();

    bool changed = false;
    // Use RPO order to improve the quality of load elimination across blocks.
    for (auto *BB : llvh::reverse(PO_)) {
      changed |= eliminateLoads(BB);
      changed |= eliminateStores(BB);
    }

    // Delete any allocas that did not end up being useful.
    deleteUnusedAllocas();
    return changed;
  }
};

bool runFrameLoadStoreOpts(Module *M) {
  CapturedVariables cv;
  // Collect information about all capturing loads and stores for every
  // variable in the module.
  for (Function &F : *M) {
    for (Variable *V : F.getFunctionScope()->getVariables()) {
      for (Instruction *I : V->getUsers()) {
        if (I->getParent()->getParent() != &F) {
          if (llvh::isa<LoadFrameInst>(I)) {
            cv.loads.insert(V);
          } else {
            assert(llvh::isa<StoreFrameInst>(I) && "No other valid user");
            cv.stores.insert(V);
          }
        }
      }
    }
  }

  bool changed = false;
  for (auto &F : *M)
    changed |= FunctionLoadStoreOptimizer(&F, cv).run();
  return changed;
}

} // namespace

Pass *createFrameLoadStoreOpts() {
  class ThisPass : public ModulePass {
   public:
    explicit ThisPass() : ModulePass("FrameLoadStoreOpts") {}
    ~ThisPass() override = default;

    bool runOnModule(Module *M) override {
      return runFrameLoadStoreOpts(M);
    }
  };
  return new ThisPass();
}

} // namespace hermes
#undef DEBUG_TYPE
