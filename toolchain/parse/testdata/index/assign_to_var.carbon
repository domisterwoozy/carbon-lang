// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// AUTOUPDATE

var v: i32 = t[0];

// CHECK:STDOUT: - filename: assign_to_var.carbon
// CHECK:STDOUT:   parse_tree: [
// CHECK:STDOUT:     {kind: 'FileStart', text: ''},
// CHECK:STDOUT:       {kind: 'VariableIntroducer', text: 'var'},
// CHECK:STDOUT:         {kind: 'Name', text: 'v'},
// CHECK:STDOUT:         {kind: 'IntegerTypeLiteral', text: 'i32'},
// CHECK:STDOUT:       {kind: 'PatternBinding', text: ':', subtree_size: 3},
// CHECK:STDOUT:       {kind: 'VariableInitializer', text: '='},
// CHECK:STDOUT:           {kind: 'NameExpr', text: 't'},
// CHECK:STDOUT:         {kind: 'IndexExprStart', text: '[', subtree_size: 2},
// CHECK:STDOUT:         {kind: 'IntegerLiteral', text: '0'},
// CHECK:STDOUT:       {kind: 'IndexExpr', text: ']', subtree_size: 4},
// CHECK:STDOUT:     {kind: 'VariableDecl', text: ';', subtree_size: 10},
// CHECK:STDOUT:     {kind: 'FileEnd', text: ''},
// CHECK:STDOUT:   ]
