#pragma once

#include <functional>
#include "AST.h"

namespace fire::AST {

enum ASTWalkerLocation {
  AW_Begin,
  AW_End,
};

void walk_ast(ASTPointer ast, std::function<bool(ASTWalkerLocation, ASTPointer)> fn);

}