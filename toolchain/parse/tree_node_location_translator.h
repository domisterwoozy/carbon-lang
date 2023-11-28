// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_PARSE_TREE_NODE_LOCATION_TRANSLATOR_H_
#define CARBON_TOOLCHAIN_PARSE_TREE_NODE_LOCATION_TRANSLATOR_H_

#include <cstdint>

#include "llvm/ADT/SmallSet.h"
#include "toolchain/diagnostics/diagnostic.h"
#include "toolchain/lex/tokenized_buffer.h"
#include "toolchain/parse/tree.h"

namespace Carbon::Parse {

class NodeLocationTranslator : public DiagnosticLocationTranslator<Node> {
 public:
  explicit NodeLocationTranslator(const Lex::TokenizedBuffer* tokens,
                                  llvm::StringRef filename,
                                  const Tree* parse_tree)
      : tokens_(tokens),
        token_translator_(tokens),
        filename_(filename),
        parse_tree_(parse_tree) {}

  // Map the given token into a diagnostic location.
  auto GetLocation(Node node) -> DiagnosticLocation override {
    // Support the invalid token as a way to emit only the filename, when there
    // is no line association.
    if (!node.is_valid()) {
      return {.file_name = filename_};
    }

    // Retrieve all tokens that descend from this node (including the root).
    // This will always represent a contiguous chunk of source b/c XXXX. Then
    // construct a location that represents this range.
    Lex::Token start_token = parse_tree_->node_token(node);
    Lex::Token end_token = start_token;
    int32_t length = 0;
    for (Node descendant : parse_tree_->postorder(node)) {
      Lex::Token desc_token = parse_tree_->node_token(descendant);
      if (!desc_token.is_valid()) {
        continue;
      }

      length += tokens_->GetTokenText(desc_token).size() +
                (tokens_->HasTrailingWhitespace(desc_token) ? 1 : 0);

      if (desc_token.index < start_token.index) {
        start_token = desc_token;
      } else if (desc_token.index > end_token.index) {
        end_token = desc_token;
      }
    }

    DiagnosticLocation loc = token_translator_.GetLocation(start_token);
    if (start_token == end_token) {
      return loc;
    }

    loc.lines = tokens_->GetLineRangeText(tokens_->GetLine(start_token),
                                          tokens_->GetEndLine(end_token));
    loc.position.length = length;
    return loc;
  }

 private:
  const Lex::TokenizedBuffer* tokens_;
  Lex::TokenLocationTranslator token_translator_;
  llvm::StringRef filename_;
  const Tree* parse_tree_;
};

}  // namespace Carbon::Parse

#endif  // CARBON_TOOLCHAIN_PARSE_TREE_NODE_LOCATION_TRANSLATOR_H_
