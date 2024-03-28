/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "hermes/BCGen/HBC/BytecodeGenerator.h"

#include "hermes/FrontEndDefs/Builtins.h"

#include "llvh/ADT/SmallString.h"
#include "llvh/Support/Format.h"

#include <unordered_map>

namespace hermes {
namespace hbc {

unsigned BytecodeFunctionGenerator::getStringID(LiteralString *value) const {
  return BMGen_.getStringID(value->getValue().str());
}

unsigned BytecodeFunctionGenerator::getIdentifierID(
    LiteralString *value) const {
  return BMGen_.getIdentifierID(value->getValue().str());
}

uint32_t BytecodeFunctionGenerator::addBigInt(bigint::ParsedBigInt bigint) {
  assert(
      !complete_ &&
      "Cannot modify BytecodeFunction after call to bytecodeGenerationComplete.");
  return BMGen_.addBigInt(std::move(bigint));
}

uint32_t BytecodeFunctionGenerator::addRegExp(CompiledRegExp *regexp) {
  assert(
      !complete_ &&
      "Cannot modify BytecodeFunction after call to bytecodeGenerationComplete.");
  return BMGen_.addRegExp(regexp);
}

uint32_t BytecodeFunctionGenerator::addFilename(llvh::StringRef filename) {
  assert(
      !complete_ &&
      "Cannot modify BytecodeFunction after call to bytecodeGenerationComplete.");
  return BMGen_.addFilename(filename);
}

void BytecodeFunctionGenerator::addExceptionHandler(
    HBCExceptionHandlerInfo info) {
  assert(
      !complete_ &&
      "Cannot modify BytecodeFunction after call to bytecodeGenerationComplete.");
  exceptionHandlers_.push_back(info);
}

void BytecodeFunctionGenerator::addDebugSourceLocation(
    const DebugSourceLocation &info) {
  assert(
      !complete_ &&
      "Cannot modify BytecodeFunction after call to bytecodeGenerationComplete.");
  // If an address is repeated, it means no actual bytecode was emitted for the
  // previous source location.
  if (!debugLocations_.empty() &&
      debugLocations_.back().address == info.address) {
    debugLocations_.back() = info;
  } else {
    debugLocations_.push_back(info);
  }
}

void BytecodeFunctionGenerator::setJumpTable(
    std::vector<uint32_t> &&jumpTable) {
  assert(
      !complete_ &&
      "Cannot modify BytecodeFunction after call to bytecodeGenerationComplete.");
  assert(!jumpTable.empty() && "invoked with no jump table");

  jumpTable_ = std::move(jumpTable);
}

std::unique_ptr<BytecodeFunction>
BytecodeFunctionGenerator::generateBytecodeFunction(
    Function::ProhibitInvoke prohibitInvoke,
    bool strictMode,
    uint32_t paramCount,
    uint32_t nameID) {
  if (!complete_) {
    bytecodeGenerationComplete();
  }

  FunctionHeader header{
      bytecodeSize_,
      paramCount,
      frameSize_,
      nameID,
      highestReadCacheIndex_,
      highestWriteCacheIndex_};

  switch (prohibitInvoke) {
    case Function::ProhibitInvoke::ProhibitNone:
      header.flags.prohibitInvoke = FunctionHeaderFlag::ProhibitNone;
      break;
    case Function::ProhibitInvoke::ProhibitConstruct:
      header.flags.prohibitInvoke = FunctionHeaderFlag::ProhibitConstruct;
      break;
    case Function::ProhibitInvoke::ProhibitCall:
      header.flags.prohibitInvoke = FunctionHeaderFlag::ProhibitCall;
      break;
  }

  header.flags.strictMode = strictMode;
  header.flags.hasExceptionHandler = exceptionHandlers_.size();

  return std::make_unique<BytecodeFunction>(
      std::move(opcodes_), std::move(header), std::move(exceptionHandlers_));
}

unsigned BytecodeFunctionGenerator::getFunctionID(Function *F) {
  return BMGen_.addFunction(F);
}

void BytecodeFunctionGenerator::shrinkJump(offset_t loc) {
  // We are shrinking a long jump into a short jump.
  // The size of operand reduces from 4 bytes to 1 byte, a delta of 3.
  opcodes_.erase(opcodes_.begin() + loc, opcodes_.begin() + loc + 3);

  // Change this instruction from long jump to short jump.
  longToShortJump(loc - 1);
}

void BytecodeFunctionGenerator::updateJumpTarget(
    offset_t loc,
    int newVal,
    int bytes) {
  // The jump target is encoded in little-endian. Update it correctly
  // regardless of host byte order.
  for (; bytes; --bytes, ++loc) {
    opcodes_[loc] = (opcode_atom_t)(newVal);
    newVal >>= 8;
  }
}

void BytecodeFunctionGenerator::updateJumpTableOffset(
    offset_t loc,
    uint32_t jumpTableOffset,
    uint32_t instLoc) {
  assert(opcodes_.size() > instLoc && "invalid switchimm offset");

  // The offset is not aligned, but will be aligned when read in the
  // interpreter.
  updateJumpTarget(
      loc,
      opcodes_.size() + jumpTableOffset * sizeof(uint32_t) - instLoc,
      sizeof(uint32_t));
}

void BytecodeFunctionGenerator::bytecodeGenerationComplete() {
  assert(!complete_ && "Can only call bytecodeGenerationComplete once");
  complete_ = true;
  bytecodeSize_ = opcodes_.size();

  // Add the jump tables inline with the opcodes, as a 4-byte aligned section at
  // the end of the opcode array.
  if (!jumpTable_.empty()) {
    uint32_t alignedOpcodes = llvh::alignTo<sizeof(uint32_t)>(bytecodeSize_);
    uint32_t jumpTableBytes = jumpTable_.size() * sizeof(uint32_t);
    opcodes_.reserve(alignedOpcodes + jumpTableBytes);
    opcodes_.resize(alignedOpcodes, 0);
    const opcode_atom_t *jumpTableStart =
        reinterpret_cast<opcode_atom_t *>(jumpTable_.data());
    opcodes_.insert(
        opcodes_.end(), jumpTableStart, jumpTableStart + jumpTableBytes);
  }
}

unsigned BytecodeModuleGenerator::addFunction(Function *F) {
  lazyFunctions_ |= F->isLazy();
  asyncFunctions_ |= llvh::isa<AsyncFunction>(F);
  return functionIDMap_.allocate(F);
}

void BytecodeModuleGenerator::setFunctionGenerator(
    Function *F,
    unique_ptr<BytecodeFunctionGenerator> BFG) {
  assert(
      functionGenerators_.find(F) == functionGenerators_.end() &&
      "Adding same function twice.");
  assert(
      !BFG->hasEncodingError() && "Error should have been reported already.");
  functionGenerators_[F] = std::move(BFG);
}

unsigned BytecodeModuleGenerator::getStringID(llvh::StringRef str) const {
  return stringTable_.getStringID(str);
}

unsigned BytecodeModuleGenerator::getIdentifierID(llvh::StringRef str) const {
  return stringTable_.getIdentifierID(str);
}

void BytecodeModuleGenerator::initializeStringTable(
    StringLiteralTable stringTable) {
  assert(stringTable_.empty() && "String table must be empty");
  stringTable_ = std::move(stringTable);
}

uint32_t BytecodeModuleGenerator::addBigInt(bigint::ParsedBigInt bigint) {
  return bigIntTable_.addBigInt(std::move(bigint));
}

void BytecodeModuleGenerator::initializeSerializedLiterals(
    LiteralBufferBuilder::Result &&bufs) {
  assert(
      arrayBuffer_.empty() && objKeyBuffer_.empty() && objValBuffer_.empty() &&
      literalOffsetMap_.empty() && "serialized literals already initialized");
  arrayBuffer_ = std::move(bufs.arrayBuffer);
  objKeyBuffer_ = std::move(bufs.keyBuffer);
  objValBuffer_ = std::move(bufs.valBuffer);
  literalOffsetMap_ = std::move(bufs.offsetMap);
}

uint32_t BytecodeModuleGenerator::addRegExp(CompiledRegExp *regexp) {
  return regExpTable_.addRegExp(regexp);
}

uint32_t BytecodeModuleGenerator::addFilename(llvh::StringRef filename) {
  return filenameTable_.addFilename(filename);
}

void BytecodeModuleGenerator::addCJSModule(
    uint32_t functionID,
    uint32_t nameID) {
  assert(
      cjsModulesStatic_.empty() &&
      "Statically resolved modules must be in cjsModulesStatic_");
  cjsModules_.push_back({nameID, functionID});
}

void BytecodeModuleGenerator::addCJSModuleStatic(
    uint32_t moduleID,
    uint32_t functionID) {
  assert(cjsModules_.empty() && "Unresolved modules must be in cjsModules_");
  cjsModulesStatic_.push_back({moduleID, functionID});
}

void BytecodeModuleGenerator::addFunctionSource(
    uint32_t functionID,
    uint32_t stringID) {
  functionSourceTable_.push_back({functionID, stringID});
}

std::unique_ptr<BytecodeModule> BytecodeModuleGenerator::generate() {
  assert(
      valid_ &&
      "BytecodeModuleGenerator::generate() cannot be called more than once");
  valid_ = false;

  assert(
      functionIDMap_.getElements().size() == functionGenerators_.size() &&
      "Missing functions.");

  BytecodeOptions bytecodeOptions;
  bytecodeOptions.hasAsync = asyncFunctions_;
  bytecodeOptions.staticBuiltins = options_.staticBuiltinsEnabled;
  bytecodeOptions.cjsModulesStaticallyResolved = !cjsModulesStatic_.empty();
  std::unique_ptr<BytecodeModule> BM{new BytecodeModule(
      functionGenerators_.size(),
      std::move(stringTable_),
      bigIntTable_.getEntryList(),
      bigIntTable_.getDigitsBuffer(),
      std::move(regExpTable_),
      entryPointIndex_,
      std::move(arrayBuffer_),
      std::move(objKeyBuffer_),
      std::move(objValBuffer_),
      segmentID_,
      std::move(cjsModules_),
      std::move(cjsModulesStatic_),
      std::move(functionSourceTable_),
      bytecodeOptions)};

  DebugInfoGenerator debugInfoGen{std::move(filenameTable_)};

  const uint32_t strippedFunctionNameId =
      options_.stripFunctionNames ? BM->getStringID(kStrippedFunctionName) : 0;
  auto functions = functionIDMap_.getElements();
  for (unsigned i = 0, e = functions.size(); i < e; ++i) {
    auto *F = functions[i];
    auto &BFG = *functionGenerators_[F];

    uint32_t functionNameId = options_.stripFunctionNames
        ? strippedFunctionNameId
        : BM->getStringID(functions[i]->getOriginalOrInferredName().str());

    std::unique_ptr<BytecodeFunction> func = BFG.generateBytecodeFunction(
        F->getProhibitInvoke(),
        F->isStrictMode(),
        F->getExpectedParamCountIncludingThis(),
        functionNameId);

    if (F->isLazy()) {
      hermes_fatal("lazy compilation not supported");
    }

    if (BFG.hasDebugInfo()) {
      uint32_t sourceLocOffset = debugInfoGen.appendSourceLocations(
          BFG.getSourceLocation(), i, BFG.getDebugLocations());
      uint32_t lexicalDataOffset = debugInfoGen.appendLexicalData(
          BFG.getLexicalParentID(), BFG.getDebugVariableNames());
      func->setDebugOffsets({sourceLocOffset, lexicalDataOffset});
    }
    BM->setFunction(i, std::move(func));
  }

  BM->setDebugInfo(debugInfoGen.serializeWithMove());
  return BM;
}

} // namespace hbc
} // namespace hermes
