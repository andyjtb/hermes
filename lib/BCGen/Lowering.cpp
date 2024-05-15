/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "hermes/BCGen/Lowering.h"
#include "hermes/BCGen/SerializedLiteralGenerator.h"
#include "hermes/IR/Analysis.h"
#include "hermes/IR/CFG.h"
#include "hermes/IR/IR.h"
#include "hermes/IR/IRBuilder.h"
#include "hermes/IR/Instrs.h"
#include "hermes/Inst/Inst.h"

#define DEBUG_TYPE "lowering"

using namespace hermes;

bool SwitchLowering::runOnFunction(Function *F) {
  bool changed = false;
  llvh::SmallVector<SwitchInst *, 4> switches;
  // Collect all switch instructions.
  for (BasicBlock &BB : *F)
    for (auto &it : BB) {
      if (auto *S = llvh::dyn_cast<SwitchInst>(&it))
        switches.push_back(S);
    }

  for (auto *S : switches) {
    lowerSwitchIntoIfs(S);
    changed = true;
  }

  return changed;
}

void SwitchLowering::lowerSwitchIntoIfs(SwitchInst *switchInst) {
  IRBuilder builder(switchInst->getParent()->getParent());
  builder.setLocation(switchInst->getLocation());

  BasicBlock *defaultDest = switchInst->getDefaultDestination();
  BasicBlock *next = defaultDest;
  BasicBlock *currentBlock = switchInst->getParent();

  // In this loop we are generating a sequence of IFs in reverse. We start
  // with the last IF that points to the Default case, and go back until we
  // generate the first IF. Then we connect the first IF into the entry
  // block and delete the Switch instruction.
  for (unsigned i = 0, e = switchInst->getNumCasePair(); i < e; ++i) {
    // Create an IF statement that matches the i'th case.
    BasicBlock *ifBlock = builder.createBasicBlock(currentBlock->getParent());

    // We scan the basic blocks in reverse!
    unsigned idx = (e - i - 1);
    auto caseEntry = switchInst->getCasePair(idx);

    builder.setInsertionBlock(ifBlock);
    auto *pred = builder.createBinaryOperatorInst(
        caseEntry.first,
        switchInst->getInputValue(),
        ValueKind::BinaryStrictlyEqualInstKind);
    // Cond branch - if the predicate of the comparison is true then jump
    // into the destination block. Otherwise jump to the next comparison in
    // the chain.
    builder.createCondBranchInst(pred, caseEntry.second, next);

    // Update any phis in the destination block.
    copyPhiTarget(caseEntry.second, currentBlock, ifBlock);

    if (next == defaultDest && caseEntry.second != next) {
      // If this block is responsible for jumps to the default block (true on
      // the first iteration), and the default block is distinct from the
      // destination of this block (which we have already updated) update phi
      // nodes in the default block too.
      copyPhiTarget(next, currentBlock, ifBlock);
    }

    next = ifBlock;
  }

  // Erase the phi edges that previously came from this block.
  erasePhiTarget(defaultDest, currentBlock);
  for (unsigned i = 0, e = switchInst->getNumCasePair(); i < e; ++i) {
    erasePhiTarget(switchInst->getCasePair(i).second, currentBlock);
  }

  switchInst->eraseFromParent();
  builder.setInsertionBlock(currentBlock);
  builder.createBranchInst(next);
}

/// Copy all incoming phi edges from a block to a new one
void SwitchLowering::copyPhiTarget(
    BasicBlock *block,
    BasicBlock *previousBlock,
    BasicBlock *newBlock) {
  for (auto &inst : *block) {
    auto *phi = llvh::dyn_cast<PhiInst>(&inst);
    if (!phi)
      break; // Phi must be first, so we won't find any more.

    Value *currentValue = nullptr;
    for (int i = 0, e = phi->getNumEntries(); i < e; i++) {
      auto pair = phi->getEntry(i);
      if (pair.second != previousBlock)
        continue;
      currentValue = pair.first;
      break;
    }

    if (currentValue) {
      phi->addEntry(currentValue, newBlock);
    }
  }
}

void SwitchLowering::erasePhiTarget(BasicBlock *block, BasicBlock *toDelete) {
  for (auto &inst : *block) {
    auto *phi = llvh::dyn_cast<PhiInst>(&inst);
    if (!phi)
      break; // Phi must be first, so we won't find any more.

    for (signed i = (signed)phi->getNumEntries() - 1; i >= 0; i--) {
      auto pair = phi->getEntry(i);
      if (pair.second != toDelete)
        continue;
      phi->removeEntry(i);
      // Some codegen can add multiple identical entries, so keep looking.
    }
  }
}

/// Starting from the given \p entry block, use the given DominanceInfo to
/// examine all blocks that satisfy \p pred and attempt to construct the longest
/// possible ordered chain of blocks such that each block dominates the block
/// after it. This is done by traversing the dominance tree, until we encounter
/// two blocks that satisfy pred and do not have a dominance relationship. Note
/// that the last block in the chain will dominate all remaining blocks that
/// satisfy \p pred.
/// \return the longest ordered chain of blocks that satisfy \p pred.
template <typename Func>
static llvh::SmallVector<BasicBlock *, 4> orderBlocksByDominance(
    const DominanceInfo &DI,
    BasicBlock *entry,
    Func &&pred) {
  class OrderBlocksContext
      : public DomTreeDFS::Visitor<OrderBlocksContext, DomTreeDFS::StackNode> {
    /// The given predicate to determine whether a block should be considered.
    Func pred_;

    /// When we encounter branching, i.e. for a given basic block, if multiple
    /// of the basic blocks dominated by that basic block all contain users of
    /// allocInst_, we can not append any of those basic blocks to
    /// sortedBasicBlocks_. Furthermore, we can not append any other basic
    /// blocks to sortedBasicBlocks_ because the branch already exists.
    bool stopAddingBasicBlock_{false};

    /// List of basic blocks that satisfy the predicate, ordered by dominance
    /// relationship.
    llvh::SmallVector<BasicBlock *, 4> sortedBasicBlocks_{};

   public:
    OrderBlocksContext(
        const DominanceInfo &DI,
        BasicBlock *entryBlock,
        Func &&pred)
        : DomTreeDFS::Visitor<OrderBlocksContext, DomTreeDFS::StackNode>(DI),
          pred_(std::forward<Func>(pred)) {
      // Perform the DFS to populate sortedBasicBlocks_.
      this->DFS(this->DT_.getNode(entryBlock));
    }

    llvh::SmallVector<BasicBlock *, 4> get() && {
      return std::move(sortedBasicBlocks_);
    }

    /// Called by DFS recursively to process each node. Note that the return
    /// value isn't actually used.
    bool processNode(DomTreeDFS::StackNode *SN) {
      BasicBlock *BB = SN->node()->getBlock();
      // If BB does not satisfy the predicate, proceed to the next block.
      if (!pred_(BB))
        return false;

      while (!sortedBasicBlocks_.empty() &&
             !this->DT_.properlyDominates(sortedBasicBlocks_.back(), BB)) {
        // If the last basic block in the list does not dominate BB,
        // it means BB and that last basic block are in parallel branches
        // of previous basic blocks. We cannot doing any lowering into
        // any of these basic blocks. So we roll back one basic block,
        // and mark the fact that we can no longer append any more basic blocks
        // afterwards because of the existence of basic blocks.
        // The DFS process needs to continue, as we may roll back even more
        // basic blocks.
        sortedBasicBlocks_.pop_back();
        stopAddingBasicBlock_ = true;
      }
      if (!stopAddingBasicBlock_) {
        sortedBasicBlocks_.push_back(BB);
        return true;
      }
      return false;
    }
  };

  return OrderBlocksContext(DI, entry, std::forward<Func>(pred)).get();
}

LowerAllocObject::StoreList LowerAllocObject::collectStores(
    AllocObjectInst *allocInst,
    const BlockUserMap &userBasicBlockMap,
    const DominanceInfo &DI) {
  // Sort the basic blocks that contain users of allocInst by dominance.
  auto sortedBlocks = orderBlocksByDominance(
      DI, allocInst->getParent(), [&userBasicBlockMap](BasicBlock *BB) {
        return userBasicBlockMap.find(BB) != userBasicBlockMap.end();
      });

  // Iterate over the sorted blocks to collect StoreNewOwnPropertyInst users
  // until we encounter a nullptr indicating we should stop.
  StoreList instrs;
  for (BasicBlock *BB : sortedBlocks) {
    for (StoreNewOwnPropertyInst *I : userBasicBlockMap.find(BB)->second) {
      // If I is null, we cannot consider additional stores.
      if (!I)
        return instrs;
      instrs.push_back(I);
    }
  }
  return instrs;
}

bool LowerAllocObject::runOnFunction(Function *F) {
  /// If we can still append to \p stores, check if the user \p U is an eligible
  /// store to \p A. If so, append it to \p stores, if not, append nullptr to
  /// indicate that subsequent users in the basic block should not be
  /// considered.
  auto tryAdd = [](AllocObjectInst *A, Instruction *U, StoreList &stores) {
    // If the store list has been terminated by a nullptr, we have already
    // encountered a non-SNOP user of A in this block. Ignore this user.
    if (!stores.empty() && !stores.back())
      return;
    auto *SI = llvh::dyn_cast<StoreNewOwnPropertyInst>(U);
    if (!SI || SI->getStoredValue() == A || !SI->getIsEnumerable()) {
      // A user that's not a StoreNewOwnPropertyInst storing into the object
      // created by allocInst. We have to stop processing here. Note that we
      // check the stored value instead of the target object so that we omit
      // the case where an object is stored into itself. While it should
      // technically be safe, this maintains the invariant that stop as soon
      // the allocated object is used as something other than the target of a
      // StoreNewOwnPropertyInst.
      stores.push_back(nullptr);
    } else {
      assert(
          SI->getObject() == A &&
          "SNOP using allocInst must use it as object or value");
      stores.push_back(SI);
    }
  };

  // For each basic block, collect an ordered list of stores into
  // AllocObjectInsts that should be considered for lowering into a buffer.
  llvh::DenseMap<AllocObjectInst *, BlockUserMap> allocUsers;
  for (BasicBlock &BB : *F)
    for (Instruction &I : BB)
      for (size_t i = 0; i < I.getNumOperands(); ++i)
        if (auto *A = llvh::dyn_cast<AllocObjectInst>(I.getOperand(i)))
          tryAdd(A, &I, allocUsers[A][&BB]);

  bool changed = false;
  DominanceInfo DI(F);
  for (const auto &[A, userBasicBlockMap] : allocUsers) {
    // Collect the stores that are guaranteed to execute before any other user
    // of this object.
    auto stores = collectStores(A, userBasicBlockMap, DI);
    changed |= lowerAllocObjectBuffer(A, stores, UINT16_MAX);
  }

  return changed;
}

static bool canSerialize(Value *V) {
  if (auto *LCI = llvh::dyn_cast_or_null<HBCLoadConstInst>(V))
    return SerializedLiteralGenerator::isSerializableLiteral(LCI->getConst());
  return false;
}

bool LowerAllocObject::lowerAllocObjectBuffer(
    AllocObjectInst *allocInst,
    const StoreList &users,
    uint32_t maxSize) {
  uint32_t size = users.size();
  // Skip processing for objects that contain 0 properties.
  if (size == 0) {
    return false;
  }
  size = std::min(maxSize, size);

  Function *F = allocInst->getParent()->getParent();
  IRBuilder builder(F);
  HBCAllocObjectFromBufferInst::ObjectPropertyMap prop_map;
  bool hasSeenNumericProp = false;
  for (uint32_t i = 0; i < size; ++i) {
    StoreNewOwnPropertyInst *I = users[i];
    // Property name can be either a LiteralNumber or a LiteralString.
    Literal *propKey = cast<Literal>(I->getProperty());
    if (auto *keyStr = llvh::dyn_cast<LiteralString>(propKey)) {
      assert(
          !toArrayIndex(keyStr->getValue().str()).hasValue() &&
          "LiteralString that looks like an array index should have been converted to a number.");
    }
    auto *propVal = I->getStoredValue();
    bool isNumericKey = llvh::isa<LiteralNumber>(propKey);
    hasSeenNumericProp |= isNumericKey;

    auto *loadInst = llvh::dyn_cast<HBCLoadConstInst>(I->getStoredValue());
    // Not counting undefined as literal since the parser doesn't
    // support it.
    if (canSerialize(loadInst)) {
      prop_map.push_back(
          std::pair<Literal *, Literal *>(propKey, loadInst->getConst()));
      I->eraseFromParent();
    } else {
      // Use null as placeholder.
      prop_map.push_back({propKey, builder.getLiteralNull()});
      builder.setLocation(I->getLocation());
      builder.setInsertionPoint(I);
      // Patch the object with the correct value.
      Instruction *patchingInst = nullptr;
      if (hasSeenNumericProp) {
        if (isNumericKey) {
          patchingInst = builder.createStoreOwnPropertyInst(
              propVal, allocInst, propKey, IRBuilder::PropEnumerable::Yes);
        } else {
          // For non-numeric keys, StorePropertyInst is more efficient because
          // it can be cached off the string ID.
          patchingInst =
              builder.createStorePropertyInst(propVal, allocInst, propKey);
        }
      } else {
        patchingInst = builder.createPrStoreInst(
            propVal,
            I->getObject(),
            i,
            cast<LiteralString>(propKey),
            propVal->getType().isNonPtr());
      }
      I->replaceAllUsesWith(patchingInst);
      I->eraseFromParent();
    }
  }

  // If we did not discover any StoreNewOwnPropertyInst that we can collapse
  // into a buffer-backed object, then return.
  if (prop_map.size() == 0) {
    return false;
  }

  builder.setLocation(allocInst->getLocation());
  builder.setInsertionPoint(allocInst);
  auto *alloc = builder.createHBCAllocObjectFromBufferInst(
      prop_map, allocInst->getSize());

  // HBCAllocObjectFromBuffer does not take a prototype argument. So if the
  // AllocObjectInst had a prototype set, make an explicit call to set it.
  if (!llvh::isa<EmptySentinel>(allocInst->getParentObject())) {
    builder.createCallBuiltinInst(
        BuiltinMethod::HermesBuiltin_silentSetPrototypeOf,
        {alloc, allocInst->getParentObject()});
  }
  allocInst->replaceAllUsesWith(alloc);
  allocInst->eraseFromParent();

  return true;
}

bool LowerAllocObjectLiteral::runOnFunction(Function *F) {
  bool changed = false;
  llvh::SmallVector<AllocObjectLiteralInst *, 4> allocs;
  for (BasicBlock &BB : *F) {
    // We need to increase the iterator before calling lowerAllocObjectBuffer.
    // Otherwise deleting the instruction will invalidate the iterator.
    for (auto it = BB.begin(), e = BB.end(); it != e;) {
      if (auto *A = llvh::dyn_cast<AllocObjectLiteralInst>(&*it++)) {
        changed |= lowerAllocObjectBuffer(A);
      }
    }
  }

  return changed;
}

bool LowerAllocObjectLiteral::lowerAlloc(AllocObjectLiteralInst *allocInst) {
  Function *F = allocInst->getParent()->getParent();
  IRBuilder builder(F);

  auto size = allocInst->getKeyValuePairCount();

  // Replace AllocObjectLiteral with a regular AllocObject
  builder.setLocation(allocInst->getLocation());
  builder.setInsertionPoint(allocInst);
  auto *Obj = builder.createAllocObjectInst(size, nullptr);

  for (unsigned i = 0; i < allocInst->getKeyValuePairCount(); i++) {
    Literal *key = allocInst->getKey(i);
    Value *value = allocInst->getValue(i);
    builder.createStoreNewOwnPropertyInst(
        value, allocInst, key, IRBuilder::PropEnumerable::Yes);
  }
  allocInst->replaceAllUsesWith(Obj);
  allocInst->eraseFromParent();

  return true;
}

bool LowerAllocObjectLiteral::lowerAllocObjectBuffer(
    AllocObjectLiteralInst *allocInst) {
  Function *F = allocInst->getParent()->getParent();
  IRBuilder builder(F);

  auto maxSize = (unsigned)UINT16_MAX;
  uint32_t size = allocInst->getKeyValuePairCount();
  size = std::min(maxSize, size);

  // Should not create HBCAllocObjectFromBufferInst for an object with 0
  // properties.
  if (size == 0) {
    return lowerAlloc(allocInst);
  }

  // Replace AllocObjectLiteral with HBCAllocObjectFromBufferInst
  builder.setLocation(allocInst->getLocation());
  builder.setInsertionPointAfter(allocInst);
  HBCAllocObjectFromBufferInst::ObjectPropertyMap propMap;

  bool hasSeenNumericProp = false;
  for (unsigned i = 0; i < size; i++) {
    Literal *propKey = allocInst->getKey(i);
    if (auto *keyStr = llvh::dyn_cast<LiteralString>(propKey)) {
      assert(
          !toArrayIndex(keyStr->getValue().str()).hasValue() &&
          "LiteralString that looks like an array index should have been converted to a number.");
    }
    Value *propVal = allocInst->getValue(i);
    bool isNumericKey = llvh::isa<LiteralNumber>(propKey);
    hasSeenNumericProp |= isNumericKey;
    if (SerializedLiteralGenerator::isSerializableLiteral(propVal)) {
      propMap.push_back({propKey, llvh::cast<Literal>(propVal)});
    } else {
      // Add the literal key in with a dummy placeholder value.
      propMap.push_back(
          std::pair<Literal *, Literal *>(propKey, builder.getLiteralNull()));
      // Patch the placeholder with the correct value.
      if (hasSeenNumericProp) {
        // We don't assume the runtime storage and layout of numeric properties.
        // So, if we have encountered a numeric property, we cannot store
        // directly into a slot.
        if (isNumericKey) {
          builder.createStoreOwnPropertyInst(
              propVal, allocInst, propKey, IRBuilder::PropEnumerable::Yes);
        } else {
          // For non-numeric keys, StorePropertyInst is more efficient because
          // it can be cached off the string ID.
          builder.createStorePropertyInst(propVal, allocInst, propKey);
        }
      } else {
        // If we haven't encountered a numeric property, we can store
        // directly into a slot.
        builder.createPrStoreInst(
            propVal,
            allocInst,
            i,
            cast<LiteralString>(propKey),
            propVal->getType().isNonPtr());
      }
    }
  }

  // Emit HBCAllocObjectFromBufferInst.
  // First, we reset insertion location.
  builder.setLocation(allocInst->getLocation());
  builder.setInsertionPoint(allocInst);
  auto *alloc = builder.createHBCAllocObjectFromBufferInst(
      propMap, allocInst->getKeyValuePairCount());
  allocInst->replaceAllUsesWith(alloc);
  allocInst->eraseFromParent();

  return true;
}

bool LowerNumericProperties::stringToNumericProperty(
    IRBuilder &builder,
    Instruction &Inst,
    unsigned operandIdx) {
  auto strLit = llvh::dyn_cast<LiteralString>(Inst.getOperand(operandIdx));
  if (!strLit)
    return false;

  // Check if the string looks exactly like an array index.
  auto num = toArrayIndex(strLit->getValue().str());
  if (num) {
    Inst.setOperand(builder.getLiteralNumber(*num), operandIdx);
    return true;
  }

  return false;
}

bool LowerNumericProperties::runOnFunction(Function *F) {
  IRBuilder builder(F);
  IRBuilder::InstructionDestroyer destroyer{};

  bool changed = false;
  for (BasicBlock &BB : *F) {
    for (Instruction &Inst : BB) {
      if (llvh::isa<BaseLoadPropertyInst>(&Inst)) {
        changed |= stringToNumericProperty(
            builder, Inst, LoadPropertyInst::PropertyIdx);
      } else if (llvh::isa<StorePropertyInst>(&Inst)) {
        changed |= stringToNumericProperty(
            builder, Inst, StorePropertyInst::PropertyIdx);
      } else if (llvh::isa<BaseStoreOwnPropertyInst>(&Inst)) {
        changed |= stringToNumericProperty(
            builder, Inst, StoreOwnPropertyInst::PropertyIdx);
      } else if (llvh::isa<DeletePropertyInst>(&Inst)) {
        changed |= stringToNumericProperty(
            builder, Inst, DeletePropertyInst::PropertyIdx);
      } else if (llvh::isa<StoreGetterSetterInst>(&Inst)) {
        changed |= stringToNumericProperty(
            builder, Inst, StoreGetterSetterInst::PropertyIdx);
      } else if (llvh::isa<AllocObjectLiteralInst>(&Inst)) {
        auto allocInst = cast<AllocObjectLiteralInst>(&Inst);
        for (unsigned i = 0; i < allocInst->getKeyValuePairCount(); i++) {
          changed |= stringToNumericProperty(builder, Inst, i * 2);
        }
      }
    }
  }
  return changed;
}

static llvh::SmallVector<Value *, 4> getArgumentsWithoutThis(CallInst *CI) {
  llvh::SmallVector<Value *, 4> args;
  for (size_t i = 1; i < CI->getNumArguments(); i++) {
    args.push_back(CI->getArgument(i));
  }
  return args;
}

bool LowerCalls::runOnFunction(Function *F) {
  IRBuilder::InstructionDestroyer destroyer;
  IRBuilder builder(F);
  bool changed = false;
  for (BasicBlock &BB : *F) {
    for (Instruction &I : BB) {
      if (auto *CI = llvh::dyn_cast<CallInst>(&I)) {
        unsigned argCount = CI->getNumArguments();
        if (argCount > UINT8_MAX) {
          builder.setLocation(CI->getLocation());
          builder.setInsertionPoint(CI);
          auto *replacement = builder.createHBCCallWithArgCount(
              CI->getCallee(),
              CI->getTarget(),
              CI->getEnvironment(),
              CI->getNewTarget(),
              builder.getLiteralNumber(argCount),
              CI->getThis(),
              getArgumentsWithoutThis(CI));
          CI->replaceAllUsesWith(replacement);
          destroyer.add(CI);
          changed = true;
          continue;
        }
        // HBCCallNInst can only be used when new.target is undefined.
        if (HBCCallNInst::kMinArgs <= argCount &&
            argCount <= HBCCallNInst::kMaxArgs &&
            llvh::isa<LiteralUndefined>(CI->getNewTarget())) {
          builder.setLocation(CI->getLocation());
          builder.setInsertionPoint(CI);
          HBCCallNInst *newCall = builder.createHBCCallNInst(
              CI->getCallee(),
              CI->getTarget(),
              CI->getEnvironment(),
              CI->getNewTarget(),
              CI->getThis(),
              getArgumentsWithoutThis(CI));
          newCall->setType(CI->getType());
          CI->replaceAllUsesWith(newCall);
          destroyer.add(CI);
          changed = true;
        }
      }
    }
  }
  return changed;
}

bool LimitAllocArray::runOnFunction(Function *F) {
  bool changed = false;
  for (BasicBlock &BB : *F) {
    for (Instruction &I : BB) {
      auto *inst = llvh::dyn_cast<AllocArrayInst>(&I);
      if (!inst || inst->getElementCount() == 0)
        continue;

      IRBuilder builder(F);
      builder.setInsertionPointAfter(inst);
      builder.setLocation(inst->getLocation());

      // Checks if any operand of an AllocArray is unserializable.
      // If it finds one, the loop removes it along with every operand past it.
      {
        bool seenUnserializable = false;
        unsigned ind = -1;
        unsigned i = AllocArrayInst::ElementStartIdx;
        unsigned e = inst->getElementCount() + AllocArrayInst::ElementStartIdx;
        while (i < e) {
          ind++;
          seenUnserializable |=
              inst->getOperand(i)->getKind() == ValueKind::LiteralBigIntKind ||
              inst->getOperand(i)->getKind() == ValueKind::LiteralUndefinedKind;
          if (seenUnserializable) {
            e--;
            builder.createStoreOwnPropertyInst(
                inst->getOperand(i),
                inst,
                builder.getLiteralNumber(ind),
                IRBuilder::PropEnumerable::Yes);
            inst->removeOperand(i);
            changed = true;
            continue;
          }
          i++;
        }
      }

      if (inst->getElementCount() == 0)
        continue;

      // Since we remove elements from inst until it fits in maxSize_,
      // the final addition to totalElems will make it equal maxSize_. Any
      // AllocArray past that would have all its operands removed, and add
      // 0 to totalElems.
      for (unsigned i = inst->getElementCount() - 1; i >= maxSize_; i--) {
        int operandOffset = AllocArrayInst::ElementStartIdx + i;
        builder.createStoreOwnPropertyInst(
            inst->getOperand(operandOffset),
            inst,
            builder.getLiteralNumber(i),
            IRBuilder::PropEnumerable::Yes);
        inst->removeOperand(operandOffset);
      }
      changed = true;
    }
  }
  return changed;
}

bool LowerCondBranch::isOperatorSupported(ValueKind kind) {
  switch (kind) {
    case ValueKind::BinaryLessThanInstKind: // <
    case ValueKind::BinaryLessThanOrEqualInstKind: // <=
    case ValueKind::BinaryGreaterThanInstKind: // >
    case ValueKind::BinaryGreaterThanOrEqualInstKind: // >=
    case ValueKind::BinaryStrictlyEqualInstKind:
    case ValueKind::BinaryStrictlyNotEqualInstKind:
    case ValueKind::BinaryNotEqualInstKind: // !=
    case ValueKind::BinaryEqualInstKind: // ==
      return true;
    default:
      return false;
  }
}

bool LowerCondBranch::runOnFunction(Function *F) {
  IRBuilder builder(F);
  bool changed = false;

  for (auto &BB : *F) {
    llvh::DenseMap<CondBranchInst *, CompareBranchInst *> condToCompMap;

    for (auto &I : BB) {
      auto *cbInst = llvh::dyn_cast<CondBranchInst>(&I);
      // This also matches constructors.
      if (!cbInst)
        continue;

      Value *cond = cbInst->getCondition();

      // If the condition has more than one user, we can't lower it.
      if (!cond->hasOneUser())
        continue;

      // The condition must be a binary operator.
      auto binopInst = llvh::dyn_cast<BinaryOperatorInst>(cond);
      if (!binopInst)
        continue;

      auto *LHS = binopInst->getLeftHandSide();
      auto *RHS = binopInst->getRightHandSide();

      // The condition must either be side-effect free, or it must be the
      // previous instruction.
      if (binopInst->getSideEffect().mayReadOrWorse())
        if (cbInst->getPrevNode() != binopInst)
          continue;

      // Only certain operators are supported.
      if (!isOperatorSupported(binopInst->getKind()))
        continue;

      builder.setInsertionPoint(cbInst);
      builder.setLocation(cbInst->getLocation());
      auto *cmpBranch = builder.createCompareBranchInst(
          LHS,
          RHS,
          CompareBranchInst::fromBinaryOperatorValueKind(binopInst->getKind()),
          cbInst->getTrueDest(),
          cbInst->getFalseDest());

      condToCompMap[cbInst] = cmpBranch;
      changed = true;
    }

    for (const auto &cbiter : condToCompMap) {
      auto binopInst =
          llvh::dyn_cast<BinaryOperatorInst>(cbiter.first->getCondition());

      cbiter.first->replaceAllUsesWith(condToCompMap[cbiter.first]);
      cbiter.first->eraseFromParent();
      binopInst->eraseFromParent();
    }
  }
  return changed;
}

#undef DEBUG_TYPE
