// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_H_
#define CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "llvm/ADT/Any.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "toolchain/diagnostics/diagnostic_kind.h"

namespace Carbon {

enum class DiagnosticLevel : int8_t {
  // A note, not indicating an error on its own, but possibly providing context
  // for an error.
  Note,
  Suggestion,
  // A warning diagnostic, indicating a likely problem with the program.
  Warning,
  // An error diagnostic, indicating that the program is not valid.
  Error,
};

// Provides a definition of a diagnostic. For example:
//   CARBON_DIAGNOSTIC(MyDiagnostic, Error, "Invalid code!");
//   CARBON_DIAGNOSTIC(MyDiagnostic, Warning, "Found {0}, expected {1}.",
//              llvm::StringRef, llvm::StringRef);
//
// Arguments are passed to llvm::formatv; see:
// https://llvm.org/doxygen/FormatVariadic_8h_source.html
//
// See `DiagnosticEmitter::Emit` for comments about argument lifetimes.
#define CARBON_DIAGNOSTIC(DiagnosticName, Level, Format, ...) \
  static constexpr auto DiagnosticName =                      \
      ::Carbon::Internal::DiagnosticBase<__VA_ARGS__>(        \
          ::Carbon::DiagnosticKind::DiagnosticName,           \
          ::Carbon::DiagnosticLevel::Level, Format)

// A location for a diagnostic in a file. The lifetime of a DiagnosticLocation
// is required to be less than SourceBuffer that it refers to due to the
// contained file_name and line references.
// Depending on which fields are populated, a DiagnosticLocation can represent:
// 1. An entire file.
// 2. A line in a file.
// 3. A specific char in a file.
// 4. A contiguous range of text in a file.
struct DiagnosticLocation {
  // Name of the file or buffer that this diagnostic refers to.
  llvm::StringRef file_name;
  // A reference to the line(s) where the diagnostic occurs. Always starts at
  // the beginning of a line and ends with a newline (but can contain multiple
  // newlines for a multiline range).
  llvm::StringRef lines;
  // 1-based line number.
  int32_t line_number = -1;
  // 1-based column number.
  int32_t column_number = -1;
  // A location can represent a range of text if length > 1. If the length
  // extends past the first line's size, it means we are representing a
  // multiline range.
  int32_t length = 1;
};

enum class InlineDiagnosticKind : int8_t {
  // Sometimes you want to include a location for contextual purposes only.
  Context,
  Basic,
  Emphasis,
  SuggestionAddition,
  SuggestionRemoval,
};

struct DiagnosticText {
  // The diagnostic's format string. This, along with format_args, will be
  // passed to format_fn.
  llvm::StringLiteral format;

  // A list of format arguments.
  //
  // These may be used by non-standard consumers to inspect diagnostic details
  // without needing to parse the formatted string; however, it should be
  // understood that diagnostic formats are subject to change and the llvm::Any
  // offers limited compile-time type safety. Integration tests are required.
  llvm::SmallVector<llvm::Any> format_args;
};

struct InlineDiagnosticMessage {
  // The location of the inline diagnostic.
  DiagnosticLocation location;
  // Represents 'why' we are referencing the location above. Primarily used for
  // formatting the message.
  InlineDiagnosticKind kind;
  // The message associated with the location above. Can be empty if we solely
  // want to emphasize the location without a specific message.
  std::optional<DiagnosticText> text;
};

// A message composing a diagnostic. This may be the main message, but can also
// be notes providing more information.
struct DiagnosticMessage {
  explicit DiagnosticMessage(
      DiagnosticKind kind, llvm::StringLiteral format,
      llvm::SmallVector<llvm::Any> format_args,
      std::function<std::string(const DiagnosticText&)> format_fn,
      llvm::SmallVector<InlineDiagnosticMessage> inline_messages,
      llvm::SmallVector<std::pair<DiagnosticLocation, std::string>>
          source_insertions)
      : kind(kind),
        primary_text({format, std::move(format_args)}),
        format_fn(std::move(format_fn)),
        inline_messages(std::move(inline_messages)),
        source_insertions(std::move(source_insertions)) {}

  // The diagnostic's kind.
  DiagnosticKind kind;

  // The top level message for this diagnostic (not associated with any specific
  // inline tokens).
  DiagnosticText primary_text;

  // Converts a DiagnosticText into formatted string. Can be used for
  // primary_text or ineline_messages. By default, this uses llvm::formatv.
  std::function<std::string(const DiagnosticText&)> format_fn;

  // The inline messages for this diagnostic sorted by the location.
  llvm::SmallVector<InlineDiagnosticMessage> inline_messages;

  // Text that we insert directly into the source lines when displaying inline
  // messages.
  // TODO
  llvm::SmallVector<std::pair<DiagnosticLocation, std::string>>
      source_insertions;
};

// An instance of a single error or warning.  Information about the diagnostic
// can be recorded into it for more complex consumers.
struct Diagnostic {
  // The diagnostic's level.
  DiagnosticLevel level;

  // The main error or warning.
  DiagnosticMessage message;

  // Notes that add context or supplemental information to the diagnostic.
  std::vector<DiagnosticMessage> notes;

  // Suggestions that may fix the diagnostic.
  std::vector<DiagnosticMessage> suggestions;
};

}  // namespace Carbon

#endif  // CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_H_
