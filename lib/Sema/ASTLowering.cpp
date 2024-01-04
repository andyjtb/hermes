/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ASTLowering.h"

#include "hermes/AST/RecursiveVisitor.h"
#include "hermes/Sema/FlowContext.h"
#include "hermes/Sema/SemContext.h"

namespace hermes::sema {

namespace {

class LowerAST : public ESTree::RecursionDepthTracker<LowerAST> {
  Context &astContext_;
  SemContext &semContext_;
  flow::FlowContext &flowContext_;
  Keywords &kw_;

 public:
  LowerAST(
      Context &astContext,
      SemContext &semContext,
      flow::FlowContext &flowContext)
      : astContext_(astContext),
        semContext_(semContext),
        flowContext_(flowContext),
        kw_(semContext_.kw) {}

  bool run(ESTree::Node *node) {
    ESTree::visitESTreeNodeNoReplace(*this, node);
    return astContext_.getSourceErrorManager().getErrorCount() == 0;
  }

  void recursionDepthExceeded(ESTree::Node *node) {
    astContext_.getSourceErrorManager().error(
        node->getSourceRange(),
        "al: Maximum recursion depth exceeded while lowering AST");
  }

  void visit(ESTree::Node *node) {
    (void)semContext_;
    (void)flowContext_;
    (void)kw_;
    ESTree::visitESTreeChildren(*this, node);
  }
};

} // anonymous namespace

bool lowerAST(
    Context &astContext,
    SemContext &semContext,
    flow::FlowContext &flowContext,
    ESTree::ProgramNode *root) {
  return LowerAST(astContext, semContext, flowContext).run(root);
}

} // namespace hermes::sema