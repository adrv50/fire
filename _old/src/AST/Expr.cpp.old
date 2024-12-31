#include "AST.h"
#include "alert.h"

namespace fire::AST {

// -------------------------------------
// Expr

ASTPtr<Expr> Expr::New(ASTKind kind, Token op, ASTPointer lhs, ASTPointer rhs) {
  return ASTNew<Expr>(kind, op, lhs, rhs);
}

ASTPointer Expr::Clone() const {
  return New(this->kind, this->op, this->lhs->Clone(), this->rhs->Clone());
}

Identifier* Expr::GetID() {
  return this->rhs->GetID();
}

Expr::Expr(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs)
    : Base(kind, optok),
      op(this->token),
      lhs(lhs),
      rhs(rhs) {
  this->_attr.IsExpr = true;
}

} // namespace fire::AST