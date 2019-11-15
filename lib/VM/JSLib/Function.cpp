/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

//===----------------------------------------------------------------------===//
/// \file
/// ES5.1 15.3 Initialize the Function constructor.
//===----------------------------------------------------------------------===//
#include "JSLibInternal.h"

#include "hermes/Regex/Executor.h"
#include "hermes/Regex/RegexTraits.h"
#include "hermes/VM/ArrayLike.h"
#include "hermes/VM/Callable.h"
#include "hermes/VM/Operations.h"
#include "hermes/VM/StringBuilder.h"
#include "hermes/VM/StringView.h"

namespace hermes {
namespace vm {

//===----------------------------------------------------------------------===//
/// Function.

Handle<JSObject> createFunctionConstructor(Runtime *runtime) {
  auto functionPrototype =
      Handle<Callable>::vmcast(&runtime->functionPrototype);

  auto cons = defineSystemConstructor(
      runtime,
      Predefined::getSymbolID(Predefined::Function),
      functionConstructor,
      functionPrototype,
      1,
      JSFunction::createWithNewDomain,
      CellKind::FunctionKind);

  // Function.prototype.xxx() methods.
  defineMethod(
      runtime,
      functionPrototype,
      Predefined::getSymbolID(Predefined::toString),
      nullptr,
      functionPrototypeToString,
      0);
  defineMethod(
      runtime,
      functionPrototype,
      Predefined::getSymbolID(Predefined::apply),
      nullptr,
      functionPrototypeApply,
      2);
  defineMethod(
      runtime,
      functionPrototype,
      Predefined::getSymbolID(Predefined::call),
      nullptr,
      functionPrototypeCall,
      1);
  defineMethod(
      runtime,
      functionPrototype,
      Predefined::getSymbolID(Predefined::bind),
      nullptr,
      functionPrototypeBind,
      1);

  DefinePropertyFlags dpf = DefinePropertyFlags::getDefaultNewPropertyFlags();
  dpf.writable = 0;
  dpf.enumerable = 0;
  dpf.configurable = 0;
  (void)defineMethod(
      runtime,
      functionPrototype,
      Predefined::getSymbolID(Predefined::SymbolHasInstance),
      Predefined::getSymbolID(Predefined::squareSymbolHasInstance),
      nullptr,
      functionPrototypeSymbolHasInstance,
      1,
      dpf);

  return cons;
}

CallResult<HermesValue>
functionConstructor(void *, Runtime *runtime, NativeArgs args) {
  return createDynamicFunction(runtime, args, false);
}

CallResult<HermesValue>
functionPrototypeToString(void *, Runtime *runtime, NativeArgs args) {
  GCScope gcScope{runtime};

  auto func = args.dyncastThis<Callable>();
  if (!func) {
    return runtime->raiseTypeError(
        "Can't call Function.prototype.toString() on non-callable");
  }

  SmallU16String<64> strBuf{};
  strBuf.append("function ");

  // Extract the name.
  auto propRes = JSObject::getNamed_RJS(
      func, runtime, Predefined::getSymbolID(Predefined::name));
  if (propRes == ExecutionStatus::EXCEPTION) {
    return ExecutionStatus::EXCEPTION;
  }

  // Convert the name to string, unless it is undefined.
  if (!propRes->isUndefined()) {
    auto strRes = toString_RJS(runtime, runtime->makeHandle(*propRes));
    if (strRes == ExecutionStatus::EXCEPTION) {
      return ExecutionStatus::EXCEPTION;
    }
    strRes->get()->copyUTF16String(strBuf);
  }

  // Append the named parameters.
  strBuf.append('(');

  // Extract ".length".
  auto lengthProp = Callable::extractOwnLengthProperty_RJS(func, runtime);
  if (lengthProp == ExecutionStatus::EXCEPTION)
    return ExecutionStatus::EXCEPTION;

  // The value of the property is not guaranteed to be meaningful, so clamp it
  // to [0..65535] for sanity.
  uint32_t paramCount = (uint32_t)std::min(65535.0, std::max(0.0, *lengthProp));

  for (uint32_t i = 0; i < paramCount; ++i) {
    if (i != 0)
      strBuf.append(", ");
    char buf[16];
    ::snprintf(buf, sizeof(buf), "a%u", i);
    strBuf.append(buf);
  }

  // The rest of the body.
  if (vmisa<NativeFunction>(*func)) {
    // Use [native code] here because we want to work with tools like
    // Babel which detect the string [native code] and use it to alter
    // behavior during the class transform.
    strBuf.append(") { [native code] }");
  } else {
    // Avoid using the [native code] string to prevent extra wrapping overhead
    // in, e.g., Babel's class extension mechanism.
    strBuf.append(") { [bytecode] }");
  }

  // Finally allocate a StringPrimitive.
  return StringPrimitive::create(runtime, strBuf);
}

CallResult<HermesValue>
functionPrototypeApply(void *, Runtime *runtime, NativeArgs args) {
  GCScope gcScope(runtime);
  auto func = args.dyncastThis<Callable>();
  if (LLVM_UNLIKELY(!func)) {
    return runtime->raiseTypeError("Can't apply() to non-callable");
  }

  if (args.getArg(1).isNull() || args.getArg(1).isUndefined()) {
    ScopedNativeCallFrame newFrame{runtime, 0, *func, false, args.getArg(0)};
    if (LLVM_UNLIKELY(newFrame.overflowed()))
      return runtime->raiseStackOverflow(
          Runtime::StackOverflowKind::NativeStack);
    return Callable::call(func, runtime);
  }

  auto argObj = Handle<JSObject>::dyn_vmcast(args.getArgHandle(1));
  if (LLVM_UNLIKELY(!argObj)) {
    return runtime->raiseTypeError(
        "Can't apply() with non-object arguments list");
  }

  CallResult<uint64_t> nRes = getArrayLikeLength(argObj, runtime);
  if (LLVM_UNLIKELY(nRes == ExecutionStatus::EXCEPTION)) {
    return ExecutionStatus::EXCEPTION;
  }
  if (*nRes > UINT32_MAX) {
    runtime->raiseRangeError("Too many arguments for apply");
  }
  uint32_t n = static_cast<uint32_t>(*nRes);
  ScopedNativeCallFrame newFrame{runtime, n, *func, false, args.getArg(0)};
  if (LLVM_UNLIKELY(newFrame.overflowed()))
    return runtime->raiseStackOverflow(Runtime::StackOverflowKind::NativeStack);

  // Initialize the arguments to undefined because we might allocate and cause
  // a gc while populating them.
  // TODO: look into doing this lazily.
  newFrame.fillArguments(n, HermesValue::encodeUndefinedValue());

  if (LLVM_UNLIKELY(
          createListFromArrayLike(
              argObj,
              runtime,
              n,
              [&](Runtime *, uint64_t index, PseudoHandle<> value) {
                newFrame->getArgRef(index) = value.getHermesValue();
                return ExecutionStatus::RETURNED;
              }) == ExecutionStatus::EXCEPTION)) {
    return ExecutionStatus::EXCEPTION;
  }

  return Callable::call(func, runtime);
}

CallResult<HermesValue>
functionPrototypeCall(void *, Runtime *runtime, NativeArgs args) {
  auto func = args.dyncastThis<Callable>();
  if (LLVM_UNLIKELY(!func)) {
    return runtime->raiseTypeError("Can't call() non-callable");
  }

  uint32_t argCount = args.getArgCount();
  ScopedNativeCallFrame newFrame{
      runtime, argCount ? argCount - 1 : 0, *func, false, args.getArg(0)};
  if (LLVM_UNLIKELY(newFrame.overflowed()))
    return runtime->raiseStackOverflow(Runtime::StackOverflowKind::NativeStack);
  for (uint32_t i = 1; i < argCount; ++i) {
    newFrame->getArgRef(i - 1) = args.getArg(i);
  }
  return Callable::call(func, runtime);
}

CallResult<HermesValue>
functionPrototypeBind(void *, Runtime *runtime, NativeArgs args) {
  auto target = Handle<Callable>::dyn_vmcast(args.getThisHandle());
  if (!target) {
    return runtime->raiseTypeError("Can't bind() a non-callable");
  }

  return BoundFunction::create(
      runtime, target, args.getArgCount(), args.begin());
}

CallResult<HermesValue>
functionPrototypeSymbolHasInstance(void *, Runtime *runtime, NativeArgs args) {
  /// 1. Let F be the this value.
  auto F = args.getThisHandle();
  /// 2. Return OrdinaryHasInstance(F, V).
  auto result = ordinaryHasInstance(runtime, F, args.getArgHandle(0));
  if (LLVM_UNLIKELY(result == ExecutionStatus::EXCEPTION)) {
    return ExecutionStatus::EXCEPTION;
  }
  return HermesValue::encodeBoolValue(*result);
}

} // namespace vm
} // namespace hermes
