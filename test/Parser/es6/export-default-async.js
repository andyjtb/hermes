/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// RUN: (! %hermesc -commonjs -dump-ast -pretty-json %s 2>/dev/null ) | %FileCheck --match-full-lines %s

// CHECK-LABEL:   "body": {
// CHECK-NEXT:     "type": "BlockStatement",
// CHECK-NEXT:     "body": [

export default async function foo() {
}
// CHECK-NEXT:       {
// CHECK-NEXT:         "type": "ExportDefaultDeclaration",
// CHECK-NEXT:         "declaration": {
// CHECK-NEXT:           "type": "FunctionDeclaration",
// CHECK-NEXT:           "id": {
// CHECK-NEXT:             "type": "Identifier",
// CHECK-NEXT:             "name": "foo",
// CHECK-NEXT:             "typeAnnotation": null
// CHECK-NEXT:           },
// CHECK-NEXT:           "params": [],
// CHECK-NEXT:           "body": {
// CHECK-NEXT:             "type": "BlockStatement",
// CHECK-NEXT:             "body": []
// CHECK-NEXT:           },
// CHECK-NEXT:           "returnType": null,
// CHECK-NEXT:           "generator": false,
// CHECK-NEXT:           "async": true
// CHECK-NEXT:         }
// CHECK-NEXT:       }

// CHECK-NEXT:     ]
// CHECK-NEXT:   },
