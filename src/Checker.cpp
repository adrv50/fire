#include <iostream>
#include <sstream>

#include "Checker.h"
#include "Error.h"
#include "alert.h"

namespace metro::checker {

#if _METRO_DEBUG_

static void print_idinfo(Checker& C,
                         Checker::IdentifierInfo& idinfo) {

  std::cout << utils::Format("IdentifierInfo %08X:\n", &idinfo);
  std::cout << "  " << idinfo.tok.str << std::endl;
}

#endif

void Checker::do_check() {
}

TypeInfo evaltype(ASTPointer ast) {

  using Kind = ASTKind;
  using namespace AST;

  switch (ast->kind) {

  case Kind::Variable: {

    break;
  }

  case Kind::CallFunc: {
  }
  }

  switch (ast->kind) {
  case Kind::Value:
    return ast->As<Value>()->value->type;
  }

  todo_impl;
}

void Checker::check_statement(ASTPointer ast) {

  switch (ast->kind) {}
}

std::vector<Checker::IdentifierInfo>::iterator
Checker::check_and_append(ScopeContext& S, ASTPointer ast) {
  if (ast->is_named) {
    S.ident_list.emplace_back(ast);
    return S.ident_list.end() - 1;
  }

  return S.ident_list.end();
}

Checker::ScopeContext Checker::make_scope_context(ASTPointer ast) {

  Checker::ScopeContext context{*this};

  switch (ast->kind) {
  case ASTKind::Program:
    for (auto&& x : ast->As<AST::Program>()->list) {
      if (x->kind == ASTKind::Block)
        context.append_child(this->make_scope_context(x));
      else
        this->check_and_append(context, x);
    }

    break;

  case ASTKind::Block:
    for (auto&& x : ast->As<AST::Block>()->list)
      this->check_and_append(context, x);

    break;
  }

  return context;
}

Checker::Checker(ASTPtr<AST::Program> ast)
    : root(ast),
      scope_context(*this) {
}

} // namespace metro::checker