#pragma once

#include "Parser/AST/AST.h"

namespace metro::sema {

using AST::ASTPointer;
using AST::ASTVector;

class Sema {
public:
  Sema(AST::Program root);

  TypeInfo EvalExprType(ASTPointer ast);

  void do_check();

private:
  ASTVec<AST::Enum> enums;
  ASTVec<AST::Class> classes;
  ASTVec<AST::Function> functions;

  AST::Program _root;
};

} // namespace metro::sema