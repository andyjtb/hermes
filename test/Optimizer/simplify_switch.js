/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// RUN: %hermesc -O -dump-ir %s | %FileCheckOrRegen %s --match-full-lines

switch (x) { case 2:
     case 44:while (cond);
}
switch (8) { case 2: 6
   case 44:while (cond);
}

// Auto-generated content below. Please do not modify manually.

// CHECK:function global(): undefined
// CHECK-NEXT:%BB0:
// CHECK-NEXT:  %0 = TryLoadGlobalPropertyInst (:any) globalObject: object, "x": string
// CHECK-NEXT:       SwitchInst %0: any, %BB3, 2: number, %BB1, 44: number, %BB1
// CHECK-NEXT:%BB1:
// CHECK-NEXT:  %2 = TryLoadGlobalPropertyInst (:any) globalObject: object, "cond": string
// CHECK-NEXT:       CondBranchInst %2: any, %BB2, %BB3
// CHECK-NEXT:%BB2:
// CHECK-NEXT:  %4 = TryLoadGlobalPropertyInst (:any) globalObject: object, "cond": string
// CHECK-NEXT:       CondBranchInst %4: any, %BB2, %BB3
// CHECK-NEXT:%BB3:
// CHECK-NEXT:       ReturnInst undefined: undefined
// CHECK-NEXT:function_end
