// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// AUTOUPDATE

var z: {.n: i32} = {.n = 4};

// CHECK:STDOUT: - filename: one_entry_no_comma.carbon
// CHECK:STDOUT:   parse_tree: [
// CHECK:STDOUT:     {kind: 'FileStart', text: ''},
// CHECK:STDOUT:       {kind: 'VariableIntroducer', text: 'var'},
// CHECK:STDOUT:         {kind: 'Name', text: 'z'},
// CHECK:STDOUT:           {kind: 'StructLiteralOrStructTypeLiteralStart', text: '{'},
// CHECK:STDOUT:               {kind: 'Name', text: 'n'},
// CHECK:STDOUT:             {kind: 'StructFieldDesignator', text: '.', subtree_size: 2},
// CHECK:STDOUT:             {kind: 'IntegerTypeLiteral', text: 'i32'},
// CHECK:STDOUT:           {kind: 'StructFieldType', text: ':', subtree_size: 4},
// CHECK:STDOUT:         {kind: 'StructTypeLiteral', text: '}', subtree_size: 6},
// CHECK:STDOUT:       {kind: 'PatternBinding', text: ':', subtree_size: 8},
// CHECK:STDOUT:       {kind: 'VariableInitializer', text: '='},
// CHECK:STDOUT:         {kind: 'StructLiteralOrStructTypeLiteralStart', text: '{'},
// CHECK:STDOUT:             {kind: 'Name', text: 'n'},
// CHECK:STDOUT:           {kind: 'StructFieldDesignator', text: '.', subtree_size: 2},
// CHECK:STDOUT:           {kind: 'IntegerLiteral', text: '4'},
// CHECK:STDOUT:         {kind: 'StructFieldValue', text: '=', subtree_size: 4},
// CHECK:STDOUT:       {kind: 'StructLiteral', text: '}', subtree_size: 6},
// CHECK:STDOUT:     {kind: 'VariableDecl', text: ';', subtree_size: 17},
// CHECK:STDOUT:     {kind: 'FileEnd', text: ''},
// CHECK:STDOUT:   ]
