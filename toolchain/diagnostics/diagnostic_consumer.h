// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_CONSUMER_H_
#define CARBON_TOOLCHAIN_DIAGNOSTICS_DIAGNOSTIC_CONSUMER_H_

#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "common/check.h"
#include "llvm/ADT/STLExtras.h"
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
    Print(std::move(diagnostic.message), prefix);
    for (const auto& note : diagnostic.notes) {
      Print(note);
    }
  }

 private:
  auto Print(DiagnosticMessage message, llvm::StringRef prefix = "") -> void {
    // If there are inline messages delegate to the new inline message printer.
    if (!message.inline_messages.empty()) {
      PrintInline(std::move(message), prefix);
      return;
    }

    *stream_ << message.primary_location.file_name;
    if (message.primary_location.position.line_number > 0) {
      *stream_ << ":" << message.primary_location.position.line_number;
      if (message.primary_location.position.column_number > 0) {
        *stream_ << ":" << message.primary_location.position.column_number;
      }
    }
    *stream_ << ": " << prefix << message.format_fn(message.primary_text)
             << "\n";
    // Since this consumer does not handle multiline diagnostics, retrieve only
    // the first line of the primary location.
    llvm::StringRef line = message.primary_location.lines.split('\n').first;
    if (message.primary_location.position.column_number > 0) {
      *stream_ << line << "\n";
      stream_->indent(message.primary_location.position.column_number - 1);
      *stream_ << "^";
      int underline_length =
          std::max(0, message.primary_location.position.length - 1);
      // We want to ensure that we don't underline past the end of the line in
      // case of a multiline token.
      // TODO: revisit this once we can reference multiple ranges on multiple
      // lines in a single diagnostic message.
      underline_length =
          std::min(underline_length,
                   static_cast<int32_t>(line.size()) -
                       message.primary_location.position.column_number);
      for (int i = 0; i < underline_length; ++i) {
        *stream_ << "~";
      }
      *stream_ << "\n";
    }
  }

  struct InlineMessagePrintState {
    explicit InlineMessagePrintState(InlineDiagnosticMessage msg)
        : msg(std::move(msg)),
          state(State::Queued),
          remaining_underlines(msg.location.length),
          msg_start_col(-1) {}

    InlineDiagnosticMessage msg;
    enum State {
      // Have not reached the line where the referenced code starts.
      Queued,
      // Currently underlining the referenced code.
      Underlining,
      // Currently printing the inline text.
      Printing,
      // Done processing the inline message.
      Done
    };
    State state;
    // Always valid.
    int32_t remaining_underlines;
    // Only valid when state is Printing.
    int32_t msg_start_col;
  };

  auto PrintInline(DiagnosticMessage message, llvm::StringRef prefix = "")
      -> void {
    *stream_ << prefix << message.format_fn(message.primary_text) << "\nat ";
    PrintLocation(message.primary_location);

    // Split the source into lines
    llvm::SmallVector<llvm::StringRef> lines;
    message.primary_location.lines.split(lines, '\n', /*MaxSplit=*/-1,
                                         /*KeepEmpty=*/false);
    int32_t current_line = message.primary_location.position.line_number;
    // The indent amount should be able to fit the largest line number plus a
    // single space.
    const int indent_amt =
        std::to_string(current_line + lines.size() - 1).size() + 1;

    // Move the inline messages  into our state tracking struct. The size of the
    // state tracking struct should always be small (O(1)) so we store all the
    // state in a single struct and loop over it many times.
    llvm::SmallVector<InlineMessagePrintState> to_process;
    for (InlineDiagnosticMessage& inline_msg : message.inline_messages) {
      to_process.emplace_back(std::move(inline_msg));
    }

    LineStart(indent_amt);
    *stream_ << "\n";
    for (llvm::StringRef source_line : lines) {
      // First print the current source line.
      LineStart(indent_amt, std::to_string(current_line));
      *stream_ << source_line << "\n";
      // Then check if we have new messages to process that start on this line.
      bool underlining = false;
      for (InlineMessagePrintState& state : to_process) {
        if (state.msg.location.line_number == current_line ||
            state.state == InlineMessagePrintState::State::Underlining) {
          state.state = InlineMessagePrintState::State::Underlining;
          underlining = true;
        }
      }
      if (!underlining) {
        // If there are no inline messages referring to this line of code, go to
        // the next source line.
        ++current_line;
        continue;
      }

      // Print underlines for the currently processing msgs.
      LineStart(indent_amt);
      int num_msgs_to_print = 0;
      for (int32_t col = 1; col <= static_cast<int32_t>(source_line.size());
           ++col) {
        // Can only have one underline per column, so we need to determine which
        // msg will contribute the underline.
        InlineMessagePrintState* underlining_msg = nullptr;
        for (InlineMessagePrintState& msg_state : to_process) {
          if (msg_state.state != InlineMessagePrintState::State::Underlining) {
            continue;
          }
          int32_t underline_start =
              msg_state.msg.location.line_number == current_line
                  ? msg_state.msg.location.column_number
                  : 0;
          if (col >= underline_start &&
              (underlining_msg == nullptr ||
               // Kind enum is ordered by underline priority.
               msg_state.msg.kind > underlining_msg->msg.kind)) {
            underlining_msg = &msg_state;
          }
        }
        if (underlining_msg) {
          *stream_ << UnderlineType(underlining_msg->msg.kind);
          // Check if we finished underlining this msg.
          if (--underlining_msg->remaining_underlines == 0) {
            if (underlining_msg->msg.text.has_value()) {
              underlining_msg->msg_start_col = col;
              num_msgs_to_print++;
              underlining_msg->state = InlineMessagePrintState::State::Printing;
            } else {
              // Some inline messages have no text and just underline a range of
              // source code.
              underlining_msg->state = InlineMessagePrintState::State::Done;
            }
          }
        } else {
          *stream_ << " ";
        }
      }
      *stream_ << "\n";
      if (num_msgs_to_print == 0) {
        // If we didn't finish underlining any msgs, go to the next source line.
        ++current_line;
        continue;
      }

      // Then print inline text for any currently processing msgs that have
      // finished.
      while (num_msgs_to_print > 0) {
        LineStart(indent_amt);
        int vert_bars = 0;
        InlineMessagePrintState* printing_msg = nullptr;
        for (int32_t col = 1; col <= static_cast<int32_t>(source_line.size());
             ++col) {
          // Short circuit if weve printed all vert bars and are ready to print
          // text.
          if (printing_msg) {
            break;
          }
          // If an underline finished in this column we want to print a vertical
          // bar no matter what.
          char to_print = ' ';
          for (InlineMessagePrintState& state : to_process) {
            if (state.state == InlineMessagePrintState::State::Printing &&
                col == state.msg_start_col) {
              to_print = '|';
              vert_bars++;
              // We are at the final vert bar, so we print the text.
              if (vert_bars == num_msgs_to_print) {
                printing_msg = &state;
              }
              break;
            }
          }
          *stream_ << to_print;
        }
        *stream_ << "-- " << message.format_fn(printing_msg->msg.text.value())
                 << "\n";
        num_msgs_to_print--;
      }

      // Go to the next source line.
      ++current_line;
    }
    LineStart(indent_amt);
    *stream_ << "\n";

    // There are three types of lines that are printed when dumping inline
    // messages.
    // 1. The source lines.
    // 2. The underlining lines (always directly below a source line).
    // 3. The inline text lines (always directly below an underlining line).
    // We first construct all of the lines without line numbers so that we know
    // how many digits we need to prefix each line with.
  }

  auto LineStart(int indent, std::string line_number = "") -> void {
    *stream_ << line_number;
    stream_->indent(indent - line_number.size());
    *stream_ << "|";
  }

  auto PrintLocation(const DiagnosticLocation& location) -> void {
    *stream_ << location.file_name;
    if (location.position.line_number > 0) {
      *stream_ << ":" << location.position.line_number;
      if (location.position.column_number > 0) {
        *stream_ << ":" << location.position.column_number;
      }
    }
    *stream_ << "\n";
  }

  auto UnderlineType(InlineDiagnosticKind kind) -> char {
    switch (kind) {
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
