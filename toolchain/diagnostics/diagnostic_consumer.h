// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_CONSUMER_H_
#define CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_CONSUMER_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "common/check.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "toolchain/diagnostics/diagnostic.h"

namespace Carbon {

// Receives diagnostics as they are emitted.
class DiagnosticConsumer {
 public:
  virtual ~DiagnosticConsumer() = default;

  // Handle a diagnostic.
  //
  // This relies on moves of the Diagnostic. At present, diagnostics are
  // allocated on the stack, so their lifetime is that of HandleDiagnostic.
  // However, SortingDiagnosticConsumer needs a longer lifetime, until all
  // diagnostics have been produced. As a consequence, it needs to either copy
  // or move the Diagnostic, and right now we're moving due to the overhead of
  // notes.
  //
  // At present, there is no persistent storage of diagnostics because IDEs
  // would be fine with diagnostics being printed immediately and discarded,
  // without SortingDiagnosticConsumer. If this becomes a performance issue, we
  // may want to investigate alternative ownership models that address both IDE
  // and CLI user needs.
  virtual auto HandleDiagnostic(Diagnostic diagnostic) -> void = 0;

  // Flushes any buffered input.
  virtual auto Flush() -> void {}
};

class StreamDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit StreamDiagnosticConsumer(llvm::raw_ostream& stream)
      : stream_(&stream) {}

  auto HandleDiagnostic(Diagnostic diagnostic) -> void override {
    std::string prefix;
    if (diagnostic.level == DiagnosticLevel::Error) {
      prefix = "ERROR: ";
    }
    Print(diagnostic.message, prefix);
    for (const auto& note : diagnostic.notes) {
      Print(note);
    }
  }

  auto Print(const DiagnosticMessage& message, llvm::StringRef prefix = "")
      -> void {
    *stream_ << prefix << message.format_fn(message.primary_text) << "\n";
    // Use the required first inline message as the primary messages location.
    PrintLocation(message.inline_messages[0].location);
    LineStart(std::nullopt);
    *stream_ << "\n";

    // First we determine the range of lines we are printing and construct a map
    // from line number
    //
    //

    for (const auto& [location, kind, text] : message.inline_messages) {
      llvm::SmallVector<llvm::StringRef> lines;
      location.lines.split(lines, '\n', /*MaxSplit=*/-1,
                           /*KeepEmpty=*/false);
      CARBON_CHECK(!lines.empty())
          << "Expected at least one line in an inline message.";
      int32_t line_number = location.line_number;
      int32_t remaining_underlines = location.length;
      int32_t final_underline_pos = -1;
      for (llvm::StringRef line : lines) {
        // Print the actual source line.
        LineStart(line_number);
        *stream_ << line << "\n";

        // Go to the next inline msg if this was simply a contextual line.
        if (kind == InlineDiagnosticKind::Context) {
          continue;
        }

        // Start underlining.
        LineStart(std::nullopt);
        // Only indent on the first line for multiline range.
        int32_t indent_amt = location.line_number == line_number
                                 ? location.column_number - 1
                                 : 0;
        stream_->indent(indent_amt);
        int32_t underline_amt =
            std::min(remaining_underlines, static_cast<int32_t>(line.size()));
        for (int i = 0; i < underline_amt; ++i) {
          *stream_ << UnderlineType(kind);
        }
        *stream_ << "\n";

        ++line_number;
        final_underline_pos = indent_amt + underline_amt;
        remaining_underlines -= underline_amt;
      }

      // Go to the next inline msg if there is no text to display.
      if (!text.has_value()) {
        continue;
      }

      // Add inline text.
      LineStart(std::nullopt);
      stream_->indent(final_underline_pos - 1);
      // TODO: use the formatting here once we fix the macro.
      *stream_ << "^-- " << text->format << "\n";
    }
    LineStart(std::nullopt);
    *stream_ << "\n";
  }

  auto LineStart(std::optional<int> line_number) -> void {
    if (line_number.has_value()) {
      *stream_ << line_number.value();
      // TODO: handle number of digits in the number
      stream_->indent(2);
    } else {
      stream_->indent(3);
    }
    *stream_ << "|";
  }

  auto PrintLocation(const DiagnosticLocation& location) -> void {
    *stream_ << location.file_name;
    if (location.line_number > 0) {
      *stream_ << ":" << location.line_number;
      if (location.column_number > 0) {
        *stream_ << ":" << location.column_number;
      }
    }
    *stream_ << ":\n";
  }

  auto UnderlineType(InlineDiagnosticKind kind) -> char {
    switch (kind) {
      case InlineDiagnosticKind::Context:
        return ' ';
      case InlineDiagnosticKind::Basic:
        return '~';
      case InlineDiagnosticKind::Emphasis:
        return '^';
      case InlineDiagnosticKind::SuggestionAddition:
        return '+';
      case InlineDiagnosticKind::SuggestionRemoval:
        return '-';
    }
  }

 private:
  llvm::raw_ostream* stream_;
};

inline auto ConsoleDiagnosticConsumer() -> DiagnosticConsumer& {
  static auto* consumer = new StreamDiagnosticConsumer(llvm::errs());
  return *consumer;
}

// Diagnostic consumer adaptor that tracks whether any errors have been
// produced.
class ErrorTrackingDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit ErrorTrackingDiagnosticConsumer(DiagnosticConsumer& next_consumer)
      : next_consumer_(&next_consumer) {}

  auto HandleDiagnostic(Diagnostic diagnostic) -> void override {
    seen_error_ |= diagnostic.level == DiagnosticLevel::Error;
    next_consumer_->HandleDiagnostic(std::move(diagnostic));
  }

  // Reset whether we've seen an error.
  auto Reset() -> void { seen_error_ = false; }

  // Returns whether we've seen an error since the last reset.
  auto seen_error() const -> bool { return seen_error_; }

 private:
  DiagnosticConsumer* next_consumer_;
  bool seen_error_ = false;
};

}  // namespace Carbon

#endif  // CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_CONSUMER_H_
