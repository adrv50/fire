#include "Sema/Sema.h"

namespace fire::semantics_checker {

TypeInfo Sema::EvalExpr(ASTPtr<AST::Expr> ast) {
  using Kind = ASTKind;
  using TK = TypeKind;

  auto lhs = this->EvalType(ast->lhs);
  auto rhs = this->EvalType(ast->rhs);

  bool is_same = lhs.equals(rhs);

  // 基本的な数値演算を除外
  switch (ast->kind) {
  case Kind::Add:
  case Kind::Sub:
  case Kind::Mul:
  case Kind::Div:
    if (is_same && lhs.is_numeric()) {
      return lhs;
    }

    break;

  case Kind::Mod:
  case Kind::LShift:
  case Kind::RShift:
    if (lhs.kind == TK::Int && rhs.kind == TK::Int)
      return TK::Int;

    break;

  case Kind::Bigger:
  case Kind::BiggerOrEqual:
    if (is_same && lhs.is_numeric_or_char() && rhs.is_numeric_or_char())
      return TK::Bool;

    break;

  case Kind::LogAND:
  case Kind::LogOR:
    if (lhs.kind != TypeKind::Bool || rhs.kind != TypeKind::Bool)
      break;

  case Kind::Equal:
    if (!is_same) {
      break;
    }

    return TK::Bool;

  case Kind::BitAND:
  case Kind::BitXOR:
  case Kind::BitOR:
    if (lhs.kind == TK::Int && rhs.kind == TK::Int)
      return TK::Int;

    break;
  }

  switch (ast->kind) {
  case Kind::Add:
    //
    // vector + T
    // T + vector
    //  --> append element to vector
    if (lhs.kind == TK::Vector || rhs.kind == TK::Vector)
      return TK::Vector;

    //
    // char + char  <--  Invalid
    // char + str
    // str  + char
    // str  + str
    if (lhs.is_char_or_str() && rhs.is_char_or_str())
      return TK::String;

    break;

  case Kind::Mul:
    //
    // int * str
    // str * int
    //  => str
    if (!is_same && lhs.is_hit_kind({TK::Int, TK::String}) &&
        rhs.is_hit_kind({TK::Int, TK::String}))
      return TK::String;

    // vector * int
    // int * vector
    //  => vector
    if (!is_same && lhs.is_hit_kind({TK::Int, TK::Vector}) &&
        rhs.is_hit_kind({TK::Int, TK::Vector}))
      return TK::Vector;

    break;
  }

  throw Error(ast->op, "invalid operator '" + ast->op.str + "' for '" + lhs.to_string() +
                           "' and '" + rhs.to_string() + "'");
}

} // namespace fire::semantics_checker