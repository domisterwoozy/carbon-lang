// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// AUTOUPDATE

// CHECK:STDERR: fail_too_few_element.carbon:[[@LINE+3]]:25: ERROR: Cannot initialize tuple of 2 element(s) from tuple with 1 element(s).
// CHECK:STDERR: var x: (i32, i32) = (2, );
// CHECK:STDERR:                         ^
var x: (i32, i32) = (2, );

// CHECK:STDOUT: constants {
// CHECK:STDOUT:   %.loc10_17.1: type = tuple_type (type, type)
// CHECK:STDOUT:   %.loc10_17.2: type = tuple_type (i32, i32)
// CHECK:STDOUT:   %.loc10_17.3: type = ptr_type (i32, i32)
// CHECK:STDOUT:   %.loc10_25: type = tuple_type (i32)
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: file "fail_too_few_element.carbon" {
// CHECK:STDOUT:   %.loc10_17.1: (type, type) = tuple_literal (i32, i32)
// CHECK:STDOUT:   %.loc10_17.2: type = converted %.loc10_17.1, constants.%.loc10_17.2
// CHECK:STDOUT:   %x.var: ref (i32, i32) = var x
// CHECK:STDOUT:   %x: ref (i32, i32) = bind_name x, %x.var
// CHECK:STDOUT:   %.loc10_22: i32 = int_literal 2
// CHECK:STDOUT:   %.loc10_25: (i32,) = tuple_literal (%.loc10_22)
// CHECK:STDOUT:   assign %x.var, <error>
// CHECK:STDOUT: }
