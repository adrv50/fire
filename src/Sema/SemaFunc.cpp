#include <iostream>
#include <sstream>

#include "alert.h"
#include "AST.h"

#include "Sema.h"
#include "Error.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

namespace fire::semantics_checker {

Sema::ArgumentCheckResult Sema::check_function_call_parameters(
    ASTPtr<CallFunc> call, bool isVariableArg, TypeVec const& formal,
    TypeVec const& actual) {

  if (actual.size() < formal.size()) // 少なすぎる
    return ArgumentCheckResult::TooFewArguments;

  // 可変長引数ではない
  // => 多すぎる場合エラー
  if (!isVariableArg && actual.size() > formal.size()) {
    return ArgumentCheckResult::TooManyArguments;
  }

  for (auto it = formal.begin(); auto&& act : actual)
    if (!it->equals(act))
      return {ArgumentCheckResult::TypeMismatch,
              static_cast<int>(it - formal.begin())};

  return ArgumentCheckResult::Ok;
}

Sema::CallableExprResult Sema::check_as_callable(ASTPointer ast) {

  CallableExprResult result = {.ast = ast};

  switch (ast->kind) {
  case ASTKind::Identifier: {
  }

  default:
    todo_impl;
  }

  return result;
}

} // namespace fire::semantics_checker
