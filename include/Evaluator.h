#pragma once

#include "Object.h"

namespace metro::eval {

class Evaluator {

  struct VarTable {
    std::map<std::string_view, ObjPointer> map;

    // for return-statement
    ObjPointer result = nullptr;

    VarTable() {
    }
  };

public:
  Evaluator(AST::Program ast);

  void do_eval();

  ObjPointer eval_expr(ASTPtr<AST::Expr> ast);

  ObjPointer evaluate(ASTPointer ast);

  ObjPtr<ObjCallable> eval_as_func(ASTPointer ast);

private:
  ObjPointer find_var(std::string_view name);
  ASTPtr<AST::Function> find_func(std::string_view name);

  VarTable& get_cur_vartable() {
    return *this->stack.rbegin();
  }

  VarTable& push() {
    return this->stack.emplace_back();
  }

  void pop() {
    this->stack.pop_back();
  }

  AST::Program root;
  std::vector<ASTPtr<AST::Function>> functions;

  std::vector<VarTable> stack;
};

} // namespace metro::eval