// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/diagnostics/mocks.h"

namespace Carbon {

void PrintTo(const Diagnostic& diagnostic, std::ostream* os) {
  *os << "Diagnostic{" << diagnostic.message.kind << ", ";
  PrintTo(diagnostic.level, os);
  for (const auto& [location, kind] : diagnostic.message.locations) {
    *os << ", " << location.file_name << ":" << location.line_number << ":"
        << location.column_number << ", \"";
  }
  *os << diagnostic.message.format_fn(diagnostic.message) << "\"}";
}

void PrintTo(DiagnosticLevel level, std::ostream* os) {
  switch (level) {
    case DiagnosticLevel::Note:
      *os << "Note";
      break;
    case DiagnosticLevel::Warning:
      *os << "Warning";
      break;
    case DiagnosticLevel::Error:
      *os << "Error";
      break;
    case DiagnosticLevel::Suggestion:
      *os << "Suggestion";
      break;
  }
}

}  // namespace Carbon
