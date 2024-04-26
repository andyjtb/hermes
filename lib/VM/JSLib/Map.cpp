/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

//===----------------------------------------------------------------------===//
/// \file
/// ES6.0 23.1 Initialize the Map constructor.
//===----------------------------------------------------------------------===//

#include "JSLibInternal.h"

#include "hermes/VM/StringPrimitive.h"

namespace hermes {
namespace vm {

Handle<JSObject> createMapConstructor(Runtime &runtime) {
  auto mapPrototype = Handle<JSObject>::vmcast(&runtime.mapPrototype);

  // Map.prototype.xxx methods.
  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::clear),
      nullptr,
      mapPrototypeClear,
      0);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::deleteStr),
      nullptr,
      mapPrototypeDelete,
      1);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::entries),
      nullptr,
      mapPrototypeEntries,
      0);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::forEach),
      nullptr,
      mapPrototypeForEach,
      1);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::get),
      nullptr,
      mapPrototypeGet,
      1);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::has),
      nullptr,
      mapPrototypeHas,
      1);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::keys),
      nullptr,
      mapPrototypeKeys,
      0);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::set),
      nullptr,
      mapPrototypeSet,
      2);

  {
    auto propValue = runtime.ignoreAllocationFailure(JSObject::getNamed_RJS(
        mapPrototype, runtime, Predefined::getSymbolID(Predefined::set)));
    runtime.mapPrototypeSet = std::move(propValue);
  }

  defineAccessor(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::size),
      nullptr,
      mapPrototypeSizeGetter,
      nullptr,
      false,
      true);

  defineMethod(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::values),
      nullptr,
      mapPrototypeValues,
      0);

  DefinePropertyFlags dpf = DefinePropertyFlags::getNewNonEnumerableFlags();

  {
    PseudoHandle<> propValue =
        runtime.ignoreAllocationFailure(JSObject::getNamed_RJS(
            mapPrototype,
            runtime,
            Predefined::getSymbolID(Predefined::entries)));
    runtime.mapPrototypeEntries = std::move(propValue);
    runtime.ignoreAllocationFailure(JSObject::defineOwnProperty(
        mapPrototype,
        runtime,
        Predefined::getSymbolID(Predefined::SymbolIterator),
        dpf,
        Handle<NativeFunction>::vmcast(&runtime.mapPrototypeEntries)));
  }

  dpf = DefinePropertyFlags::getDefaultNewPropertyFlags();
  dpf.writable = 0;
  dpf.enumerable = 0;
  defineProperty(
      runtime,
      mapPrototype,
      Predefined::getSymbolID(Predefined::SymbolToStringTag),
      runtime.getPredefinedStringHandle(Predefined::Map),
      dpf);

  auto cons = defineSystemConstructor<JSMap>(
      runtime,
      Predefined::getSymbolID(Predefined::Map),
      mapConstructor,
      mapPrototype,
      0,
      CellKind::JSMapKind);

  return cons;
}

/// Populate the Map with the contents of the source Map.
/// \param target the Map to populate (newly constructed).
/// \param src the Map to pull the entries from.
/// \return the newly populated map.
static ExecutionStatus
mapFromMapFastPath(Runtime &runtime, Handle<JSMap> target, Handle<JSMap> src) {
  // TODO: This can be improved further by avoiding any rehashes and the
  // SmallHermesValue unbox/boxing. We should be able to make an
  // OrderedHashMap::clone that initializes based on an existing Map
  // and clones all entries directly somehow.
  MutableHandle<> keyHandle{runtime};
  MutableHandle<> valueHandle{runtime};
  return JSMap::forEachNative(
      src,
      runtime,
      [&target, &keyHandle, &valueHandle](
          Runtime &runtime, Handle<HashMapEntry> entry) -> ExecutionStatus {
        keyHandle = entry->key.unboxToHV(runtime);
        valueHandle = entry->value.unboxToHV(runtime);
        JSMap::addValue(target, runtime, keyHandle, valueHandle);
        return ExecutionStatus::RETURNED;
      });
}

CallResult<HermesValue>
mapConstructor(void *, Runtime &runtime, NativeArgs args) {
  GCScope gcScope{runtime};
  if (LLVM_UNLIKELY(!args.isConstructorCall())) {
    return runtime.raiseTypeError("Constructor Map requires 'new'");
  }
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError("Map Constructor only applies to Map object");
  }
  JSMap::initializeStorage(selfHandle, runtime);
  if (args.getArgCount() == 0 || args.getArg(0).isUndefined() ||
      args.getArg(0).isNull()) {
    return selfHandle.getHermesValue();
  }

  auto propRes = JSObject::getNamed_RJS(
      selfHandle, runtime, Predefined::getSymbolID(Predefined::set));
  if (LLVM_UNLIKELY(propRes == ExecutionStatus::EXCEPTION)) {
    return ExecutionStatus::EXCEPTION;
  }

  // ES6.0 23.1.1.1.7: Cache adder across all iterations of the loop.
  auto adder =
      Handle<Callable>::dyn_vmcast(runtime.makeHandle(std::move(*propRes)));
  if (!adder) {
    return runtime.raiseTypeError("Property 'set' for Map is not callable");
  }

  auto iterMethodRes = getMethod(
      runtime,
      args.getArgHandle(0),
      runtime.makeHandle(Predefined::getSymbolID(Predefined::SymbolIterator)));
  if (LLVM_UNLIKELY(iterMethodRes == ExecutionStatus::EXCEPTION)) {
    return ExecutionStatus::EXCEPTION;
  }
  if (!vmisa<Callable>(iterMethodRes->getHermesValue())) {
    return runtime.raiseTypeError("iterator method is not callable");
  }
  auto iterMethod = runtime.makeHandle<Callable>(std::move(*iterMethodRes));

  // Check and run fast path.
  // If the adder is the default one, we can call JSSet::addValue directly.
  if (LLVM_LIKELY(
          adder.getHermesValue().getRaw() ==
          runtime.mapPrototypeSet.getRaw())) {
    // If the iterable is a Map with the original iterator,
    // then we can do for-loop.
    if (Handle<JSMap> inputMap = args.dyncastArg<JSMap>(0); inputMap &&
        LLVM_LIKELY(iterMethod.getHermesValue().getRaw() ==
                    runtime.mapPrototypeEntries.getRaw())) {
      if (LLVM_UNLIKELY(
              mapFromMapFastPath(runtime, selfHandle, inputMap) ==
              ExecutionStatus::EXCEPTION)) {
        return ExecutionStatus::EXCEPTION;
      }
      return selfHandle.getHermesValue();
    }

    // TODO: Fast path for JSArray input.
  }

  return addEntriesFromIterable(
      runtime,
      selfHandle,
      args.getArgHandle(0),
      iterMethod,
      [&runtime, selfHandle, adder](Runtime &, Handle<> key, Handle<> value) {
        return Callable::executeCall2(
                   adder,
                   runtime,
                   selfHandle,
                   key.getHermesValue(),
                   value.getHermesValue())
            .getStatus();
      });
}

CallResult<HermesValue>
mapPrototypeClear(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError(
        "Non-Map object called on Map.prototype.clear");
  }
  JSMap::clear(selfHandle, runtime);
  return HermesValue::encodeUndefinedValue();
}

CallResult<HermesValue>
mapPrototypeDelete(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError(
        "Non-Map object called on Map.prototype.delete");
  }
  return HermesValue::encodeBoolValue(
      JSMap::deleteKey(selfHandle, runtime, args.getArgHandle(0)));
}

CallResult<HermesValue>
mapPrototypeEntries(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError(
        "Non-Map object called on Map.prototype.entries");
  }
  auto iterator = runtime.makeHandle(JSMapIterator::create(
      runtime, Handle<JSObject>::vmcast(&runtime.mapIteratorPrototype)));
  iterator->initializeIterator(runtime, selfHandle, IterationKind::Entry);
  return iterator.getHermesValue();
}

CallResult<HermesValue>
mapPrototypeForEach(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError(
        "Non-Map object called on Map.prototype.forEach");
  }
  auto callbackfn = args.dyncastArg<Callable>(0);
  if (LLVM_UNLIKELY(!callbackfn)) {
    return runtime.raiseTypeError(
        "callbackfn must be Callable in Map.prototype.forEach");
  }
  auto thisArg = args.getArgHandle(1);
  if (JSMap::forEach(selfHandle, runtime, callbackfn, thisArg) ==
      ExecutionStatus::EXCEPTION)
    return ExecutionStatus::EXCEPTION;
  return HermesValue::encodeUndefinedValue();
}

CallResult<HermesValue>
mapPrototypeGet(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError("Non-Map object called on Map.prototype.get");
  }
  return JSMap::getValue(selfHandle, runtime, args.getArgHandle(0));
}

CallResult<HermesValue>
mapPrototypeHas(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError("Non-Map object called on Map.prototype.has");
  }
  return HermesValue::encodeBoolValue(
      JSMap::hasKey(selfHandle, runtime, args.getArgHandle(0)));
}

CallResult<HermesValue>
mapPrototypeKeys(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError(
        "Non-Map object called on Map.prototype.keys");
  }

  auto iterator = runtime.makeHandle(JSMapIterator::create(
      runtime, Handle<JSObject>::vmcast(&runtime.mapIteratorPrototype)));
  iterator->initializeIterator(runtime, selfHandle, IterationKind::Key);
  return iterator.getHermesValue();
}

// ES12 23.1.3.9 Map.prototype.set ( key, value )
CallResult<HermesValue>
mapPrototypeSet(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError("Non-Map object called on Map.prototype.set");
  }
  auto keyHandle = args.getArgHandle(0);
  // 5. If key is -0, set key to +0.
  // N.B. in the case of Map, only key should be normalized but not the value.
  auto key = keyHandle->isNumber() && keyHandle->getNumber() == 0
      ? HandleRootOwner::getZeroValue()
      : keyHandle;
  JSMap::addValue(selfHandle, runtime, key, args.getArgHandle(1));
  return selfHandle.getHermesValue();
}

CallResult<HermesValue>
mapPrototypeSizeGetter(void *, Runtime &runtime, NativeArgs args) {
  auto self = dyn_vmcast<JSMap>(args.getThisArg());
  if (LLVM_UNLIKELY(!self)) {
    return runtime.raiseTypeError(
        "Non-Map object called on Map.prototype.size");
  }
  return HermesValue::encodeTrustedNumberValue(JSMap::getSize(self, runtime));
}

CallResult<HermesValue>
mapPrototypeValues(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMap>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError(
        "Non-Map object called on Map.prototype.values");
  }
  auto iterator = runtime.makeHandle(JSMapIterator::create(
      runtime, Handle<JSObject>::vmcast(&runtime.mapIteratorPrototype)));
  iterator->initializeIterator(runtime, selfHandle, IterationKind::Value);
  return iterator.getHermesValue();
}

Handle<JSObject> createMapIteratorPrototype(Runtime &runtime) {
  auto parentHandle = runtime.makeHandle(JSObject::create(
      runtime, Handle<JSObject>::vmcast(&runtime.iteratorPrototype)));
  defineMethod(
      runtime,
      parentHandle,
      Predefined::getSymbolID(Predefined::next),
      nullptr,
      mapIteratorPrototypeNext,
      0);

  auto dpf = DefinePropertyFlags::getDefaultNewPropertyFlags();
  dpf.writable = 0;
  dpf.enumerable = 0;
  defineProperty(
      runtime,
      parentHandle,
      Predefined::getSymbolID(Predefined::SymbolToStringTag),
      runtime.getPredefinedStringHandle(Predefined::MapIterator),
      dpf);

  return parentHandle;
}

CallResult<HermesValue>
mapIteratorPrototypeNext(void *, Runtime &runtime, NativeArgs args) {
  auto selfHandle = args.dyncastThis<JSMapIterator>();
  if (LLVM_UNLIKELY(!selfHandle)) {
    return runtime.raiseTypeError(
        "Non-MapIterator object called on MapIterator.prototype.next");
  }

  auto cr = JSMapIterator::nextElement(selfHandle, runtime);
  if (LLVM_UNLIKELY(cr == ExecutionStatus::EXCEPTION)) {
    return ExecutionStatus::EXCEPTION;
  }
  return *cr;
}
} // namespace vm
} // namespace hermes
