#include <iostream>
#include <sstream>

#include "alert.h"
#include "AST.h"

#include "Checker.h"
#include "Error.h"

namespace fire::sema {

Sema::Sema(ASTPtr<AST::Program> prg)
    : root(prg) {

  for (auto&& x : prg->list) {
    switch (x->kind) {
    case ASTKind::Function:
      this->add_func(ASTCast<Function>(x));
      break;

    case ASTKind::Enum:
      this->add_enum(ASTCast<Enum>(x));
      break;

    case ASTKind::Class:
      this->add_class(ASTCast<Class>(x));
      break;
    }
  }
}

void Sema::check() {
}

Sema::ArgumentCheckResult Sema::check_function_call_parameters(
    Node<CallFunc> call, bool isVariableArg, TypeVec const& formal,
    TypeVec const& actual) {

  // 定義側
  int formal_index = 0;
  int formal_count = formal.size();

  // 呼び出し側
  int actual_index = 0;
  int actual_count = call->args.size();

  if (actual.size() < formal.size()) // 少なすぎる
    return ArgumentCheckResult::TooFewArguments;

  // 可変長引数ではない
  // => 多すぎる場合エラー
  if (!isVariableArg && actual.size() > formal.size()) {
    return ArgumentCheckResult::TooManyArguments;
  }

  for (auto it = formal.begin(); auto&& act : actual)
    if (!it->equals(act))
      return {ArgumentCheckResult::TypeMismatch, it - formal.begin()};

  return ArgumentCheckResult::Ok;
}

Sema::NameFindResult Sema::find_name(std::string const& name) {
}

TypeInfo Sema::evaltype(ASTPointer ast) {
  using Kind = ASTKind;
  using TK = TypeKind;

  if (!ast)
    return {};

  switch (ast->kind) {
  case Kind::Value:
    return ast->as_value()->value->type;

  case Kind::Variable: {

    break;
  }

  case Kind::CallFunc: {
    auto x = ASTCast<AST::CallFunc>(ast);

    auto func = this->evaltype(x->expr);

    if (func.kind != TK::Function) {
    }

    break;
  }

  case Kind::SpecifyArgumentName:
    return this->evaltype(ast->as_expr()->rhs);
  }

  if (ast->is_expr) {
    auto x = ast->as_expr();
    auto lhs = this->evaltype(x->lhs);
    auto rhs = this->evaltype(x->rhs);

    bool is_same = lhs.equals(rhs);

    // 基本的な数値演算を除外
    switch (ast->kind) {
    case Kind::Add:
    case Kind::Sub:
    case Kind::Mul:
    case Kind::Div:
      if (lhs.is_numeric() && rhs.is_numeric())
        return (lhs.kind == TK::Float || rhs.kind == TK::Float) ? TK::Float
                                                                : TK::Int;

      break;

    case Kind::Mod:
    case Kind::LShift:
    case Kind::RShift:
      if (lhs.kind == TK::Int && rhs.kind == TK::Int)
        return TK::Int;

      break;

    case Kind::Bigger:
    case Kind::BiggerOrEqual:
      if (lhs.is_numeric_or_char() && rhs.is_numeric_or_char()) {
      case Kind::Equal:
      case Kind::LogAND:
      case Kind::LogOR:
        return TK::Bool;
      }

      break;

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
      if (lhs.kind == TK::Vector || rhs.kind == TK::Vector)
        return TK::Vector;

      //
      // char + char  <--  Invalid
      // char + str
      // str  + char
      // str  + str
      if (!is_same && lhs.is_char_or_str() && rhs.is_char_or_str())
        return TK::String;

      break;

    case Kind::Mul:
      //
      // int * str
      // str * int
      //  => str
      if (!is_same && lhs.is_hit({TK::Int, TK::String}) &&
          rhs.is_hit({TK::Int, TK::String}))
        return TK::String;

      // vector * int
      // int * vector
      //  => vector
      if (lhs.is_hit({TK::Int, TK::Vector}) &&
          rhs.is_hit({TK::Int, TK::Vector}))
        return TK::Vector;

      break;
    }

  err:
    Error(x->op, "invalid operator for '" + lhs.to_string() + "' and '" +
                     rhs.to_string() + "'")();
  }

  todo_impl;
}

} // namespace fire::sema