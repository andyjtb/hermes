/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// RUN: %shermes -Werror -fno-std-globals --typed --dump-sema %s | %FileCheckOrRegen %s --match-full-lines

let a1 : number[] = [];
let a2 : number[] = a1;

type A1 = number[][];
type A2 = number[][];
type U1 = A1 | string;
type U2 = string | A2 | A1;

let a3 : U1[] = [];
let a4 : U2[] = [];

let a5: number[] = ([]: (number|number)[]);

// Auto-generated content below. Please do not modify manually.

// CHECK:%untyped_function.1 = untyped_function()
// CHECK-NEXT:%array.2 = array(%array.3)
// CHECK-NEXT:%union.4 = union(string | %array.2)
// CHECK-NEXT:%array.3 = array(number)
// CHECK-NEXT:%array.5 = array(%union.4)
// CHECK-NEXT:%object.6 = object({
// CHECK-NEXT:})

// CHECK:SemContext
// CHECK-NEXT:Func strict
// CHECK-NEXT:    Scope %s.1
// CHECK-NEXT:    Func strict
// CHECK-NEXT:        Scope %s.2
// CHECK-NEXT:            Decl %d.1 'exports' Parameter : any
// CHECK-NEXT:            Decl %d.2 'a1' Let : %array.3
// CHECK-NEXT:            Decl %d.3 'a2' Let : %array.3
// CHECK-NEXT:            Decl %d.4 'a3' Let : %array.5
// CHECK-NEXT:            Decl %d.5 'a4' Let : %array.5
// CHECK-NEXT:            Decl %d.6 'a5' Let : %array.3
// CHECK-NEXT:            Decl %d.7 'arguments' Var Arguments

// CHECK:Program Scope %s.1
// CHECK-NEXT:    ExpressionStatement
// CHECK-NEXT:        CallExpression : any
// CHECK-NEXT:            FunctionExpression : %untyped_function.1
// CHECK-NEXT:                Id 'exports' [D:E:%d.1 'exports']
// CHECK-NEXT:                BlockStatement
// CHECK-NEXT:                    VariableDeclaration
// CHECK-NEXT:                        VariableDeclarator
// CHECK-NEXT:                            ArrayExpression : %array.3
// CHECK-NEXT:                            Id 'a1' [D:E:%d.2 'a1']
// CHECK-NEXT:                    VariableDeclaration
// CHECK-NEXT:                        VariableDeclarator
// CHECK-NEXT:                            Id 'a1' [D:E:%d.2 'a1'] : %array.3
// CHECK-NEXT:                            Id 'a2' [D:E:%d.3 'a2']
// CHECK-NEXT:                    TypeAlias
// CHECK-NEXT:                        Id 'A1'
// CHECK-NEXT:                        ArrayTypeAnnotation
// CHECK-NEXT:                            ArrayTypeAnnotation
// CHECK-NEXT:                                NumberTypeAnnotation
// CHECK-NEXT:                    TypeAlias
// CHECK-NEXT:                        Id 'A2'
// CHECK-NEXT:                        ArrayTypeAnnotation
// CHECK-NEXT:                            ArrayTypeAnnotation
// CHECK-NEXT:                                NumberTypeAnnotation
// CHECK-NEXT:                    TypeAlias
// CHECK-NEXT:                        Id 'U1'
// CHECK-NEXT:                        UnionTypeAnnotation
// CHECK-NEXT:                            GenericTypeAnnotation
// CHECK-NEXT:                                Id 'A1'
// CHECK-NEXT:                            StringTypeAnnotation
// CHECK-NEXT:                    TypeAlias
// CHECK-NEXT:                        Id 'U2'
// CHECK-NEXT:                        UnionTypeAnnotation
// CHECK-NEXT:                            StringTypeAnnotation
// CHECK-NEXT:                            GenericTypeAnnotation
// CHECK-NEXT:                                Id 'A2'
// CHECK-NEXT:                            GenericTypeAnnotation
// CHECK-NEXT:                                Id 'A1'
// CHECK-NEXT:                    VariableDeclaration
// CHECK-NEXT:                        VariableDeclarator
// CHECK-NEXT:                            ArrayExpression : %array.5
// CHECK-NEXT:                            Id 'a3' [D:E:%d.4 'a3']
// CHECK-NEXT:                    VariableDeclaration
// CHECK-NEXT:                        VariableDeclarator
// CHECK-NEXT:                            ArrayExpression : %array.5
// CHECK-NEXT:                            Id 'a4' [D:E:%d.5 'a4']
// CHECK-NEXT:                    VariableDeclaration
// CHECK-NEXT:                        VariableDeclarator
// CHECK-NEXT:                            TypeCastExpression : %array.3
// CHECK-NEXT:                                ArrayExpression : %array.3
// CHECK-NEXT:                            Id 'a5' [D:E:%d.6 'a5']
// CHECK-NEXT:            ObjectExpression : %object.6
