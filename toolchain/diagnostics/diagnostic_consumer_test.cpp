// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/diagnostics/diagnostic_consumer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "toolchain/diagnostics/diagnostic.h"
#include "toolchain/diagnostics/diagnostic_kind.h"

namespace Carbon {
namespace {

TEST(InlineDiagnosticConsumerTest, PrimaryMessageOnly) {
  DiagnosticMessage msg(
      DiagnosticKind::TestDiagnostic, "primary error only",
      /*format_args=*/{},
      [](const DiagnosticText& text) -> std::string {
        return llvm::formatv(text.format.data());
      },
      {.file_name = "/file/path/f.cc",
       .lines = "source code line here",
       .position = {.line_number = 5, .column_number = 8, .length = 4}},
      /*inline_messages=*/{}, /*source_insertions=*/{});
  Diagnostic diagnostic{.level = DiagnosticLevel::Error, .message = msg};

  std::string output;
  llvm::raw_string_ostream stream(output);
  StreamDiagnosticConsumer consumer(stream);
  consumer.HandleDiagnostic(std::move(diagnostic));

  EXPECT_EQ(
      R"(/file/path/f.cc:5:8: ERROR: primary error only
source code line here
       ^~~~
)",
      output);
}

TEST(InlineDiagnosticConsumerTest, SingleInlineMessage) {
  InlineDiagnosticMessage inline_msg{
      .location = {.line_number = 5, .column_number = 10, .length = 4},
      .kind = InlineDiagnosticKind::Basic,
      .text = DiagnosticText{.format = "inline message here"}};
  DiagnosticMessage msg(DiagnosticKind::TestDiagnostic, "primary error msg",
                        /*format_args=*/{},
                        [](const DiagnosticText& text) -> std::string {
                          return llvm::formatv(text.format.data());
                        },
                        {.file_name = "/file/path/f.cc",
                         .lines = "  source code line here",
                         .position = inline_msg.location},
                        {inline_msg},
                        /*source_insertions=*/{});
  Diagnostic diagnostic{.level = DiagnosticLevel::Error, .message = msg};

  std::string output;
  llvm::raw_string_ostream stream(output);
  StreamDiagnosticConsumer consumer(stream);
  consumer.HandleDiagnostic(std::move(diagnostic));

  EXPECT_EQ(R"(ERROR: primary error msg
at /file/path/f.cc:5:10
  |
5 |  source code line here
  |         ~~~~
  |            |-- inline message here
  |
)",
            output);
}

TEST(InlineDiagnosticConsumerTest, MultilineInlineMessage) {
  InlineDiagnosticMessage inline_msg{
      .location = {.line_number = 55, .column_number = 20, .length = 20},
      .kind = InlineDiagnosticKind::Basic,
      .text = DiagnosticText{.format = "inline message here"}};
  DiagnosticMessage msg(
      DiagnosticKind::TestDiagnostic, "primary error msg",
      /*format_args=*/{},
      [](const DiagnosticText& text) -> std::string {
        return llvm::formatv(text.format.data());
      },
      {.file_name = "/file/path/f.cc",
       .lines = "  source code line '''\nmultilinetoken\n''' here",
       .position = inline_msg.location},
      {inline_msg},
      /*source_insertions=*/{});
  Diagnostic diagnostic{.level = DiagnosticLevel::Error, .message = msg};

  std::string output;
  llvm::raw_string_ostream stream(output);
  StreamDiagnosticConsumer consumer(stream);
  consumer.HandleDiagnostic(std::move(diagnostic));

  EXPECT_EQ(R"(ERROR: primary error msg
at /file/path/f.cc:55:20
   |
55 |  source code line '''
   |                   ~~~
56 |multilinetoken
   |~~~~~~~~~~~~~~
57 |''' here
   |~~~
   |  |-- inline message here
   |
)",
            output);
}

TEST(InlineDiagnosticConsumerTest, OverlappingInlineMessages) {
  // todo
}

TEST(InlineDiagnosticConsumerTest, SuggestCodeInsertion) {
  // todo
}

}  // namespace
}  // namespace Carbon
