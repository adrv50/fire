#pragma once

#include <functional>

namespace fire::AST {

enum ASTWalkerLocation {
  AW_Begin,
  AW_End,
};

void walk_ast(ASTPointer ast, std::function<void(ASTWalkerLocation, ASTPointer)> fn);

string ToString(ASTPointer ast);

template <typename T, typename U>
ASTVec<T> CloneASTVec(ASTVec<U> const& vec) {
  ASTVec<T> v;

  for (auto&& elem : vec)
    v.emplace_back(ASTCast<T>(elem->Clone()));

  return v;
}

} // namespace fire::AST