// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// AUTOUPDATE

// CHECK:STDERR: fail_assign_empty.carbon:[[@LINE+3]]:18: ERROR: Cannot initialize tuple of 1 element(s) from tuple with 0 element(s).
// CHECK:STDERR: var x: (i32,) = ();
// CHECK:STDERR:                  ^
var x: (i32,) = ();

// CHECK:STDOUT: constants {
// CHECK:STDOUT:   %.loc10_13.1: type = tuple_type (type)
// CHECK:STDOUT:   %.loc10_13.2: type = tuple_type (i32)
// CHECK:STDOUT:   %.loc10_18: type = tuple_type ()
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: file "fail_assign_empty.carbon" {
// CHECK:STDOUT:   %.loc10_13.1: (type,) = tuple_literal (i32)
// CHECK:STDOUT:   %.loc10_13.2: type = converted %.loc10_13.1, constants.%.loc10_13.2
// CHECK:STDOUT:   %x.var: ref (i32,) = var x
// CHECK:STDOUT:   %x: ref (i32,) = bind_name x, %x.var
// CHECK:STDOUT:   %.loc10_18: () = tuple_literal ()
// CHECK:STDOUT:   assign %x.var, <error>
// CHECK:STDOUT: }
