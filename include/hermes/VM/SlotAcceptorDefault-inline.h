/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef HERMES_VM_SLOTACCEPTORDEFAULT_INLINE_H
#define HERMES_VM_SLOTACCEPTORDEFAULT_INLINE_H

#include "hermes/VM/SlotAcceptorDefault.h"

#include "hermes/VM/GCPointer-inline.h"
#include "hermes/VM/WeakRef.h"

namespace hermes {
namespace vm {

inline void WeakRootAcceptorDefault::acceptWeak(WeakRootBase &ptr) {
  GCPointerBase::StorageType weakRootStorage = ptr.getNoBarrierUnsafe();
  acceptWeak(weakRootStorage);
  // Assign back to the input pointer location.
  ptr = weakRootStorage;
}

inline void SlotAcceptorDefault::accept(BasedPointer &ptr) {
  if (!ptr) {
    return;
  }
  // accept takes an l-value reference and potentially writes to it.
  // Write the value back out to the BasedPointer.
  PointerBase *const base = gc.getPointerBase();
  void *actualizedPointer = base->basedToPointerNonNull(ptr);
  accept(actualizedPointer);
  // Assign back to the based pointer.
  ptr = base->pointerToBased(actualizedPointer);
}

inline void WeakRootAcceptorDefault::acceptWeak(BasedPointer &ptr) {
  if (!ptr) {
    return;
  }
  // accept takes an l-value reference and potentially writes to it.
  // Write the value back out to the BasedPointer.
  PointerBase *const base = gcForWeakRootDefault.getPointerBase();
  void *actualizedPointer = base->basedToPointerNonNull(ptr);
  acceptWeak(actualizedPointer);
  // Assign back to the based pointer.
  ptr = base->pointerToBased(actualizedPointer);
}

} // namespace vm
} // namespace hermes

#endif // HERMES_VM_SLOTACCEPTORDEFAULT_INLINE_H
