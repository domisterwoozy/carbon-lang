// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "explorer/ast/ast_node.h"

#include "explorer/base/print_as_id.h"

namespace Carbon {

AstNode::~AstNode() = default;

void PrintTreeIndent(const AstNode& node, int indent, llvm::raw_ostream& out) {
  out.indent(indent) << "|`" << PrintAsID(node) << "` " << node.kind() << " ("
                     << node.source_loc() << ")\n";
  for (const AstNode* child : node.children()) {
    PrintTreeIndent(*child, indent + 2, out);
  }
}

void AstNode::PrintTree(llvm::raw_ostream& out) const {
  PrintTreeIndent(*this, 0, out);
}

}  // namespace Carbon
