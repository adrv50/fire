#include <iostream>
#include <sstream>
#include <list>

#include "alert.h"
#include "AST.h"

#include "Sema.h"
#include "Error.h"

namespace fire::semantics_checker {

ScopeContext* ScopeContext::find_child_scope(ASTPointer ast) const {
  return nullptr;
}

ScopeContext* FunctionScope::find_child_scope(ASTPointer ast) const {
}

ScopeContext* BlockScope::find_child_scope(ASTPointer ast) const {
}

BlockScope::BlockScope(ASTPtr<AST::Block> ast)
    : ScopeContext(SC_Block),
      ast(ast) {
  alert;

  for (auto&& e : ast->list) {
    alert;

    switch (e->kind) {
    case ASTKind::Block:
      alert;
      this->AddScope(new BlockScope(ASTCast<AST::Block>(e)));
      break;

    case ASTKind::Function:
      alert;
      this->AddScope(new FunctionScope(ASTCast<AST::Function>(e)));
      break;
    }
  }
}

ScopeContext::LocalVar& FunctionScope::add_arg(AST::Function::Argument* def) {
  auto& arg = this->arguments.emplace_back(def->get_name());

  arg.arg = def;
  arg.is_argument = true;

  if (!this->is_templated()) {
    arg.deducted_type = Sema::GetInstance()->evaltype(def->type);
    arg.is_type_deducted = true;
  }

  return arg;
}

FunctionScope::FunctionScope(ASTPtr<AST::Function> ast)
    : ScopeContext(SC_Func),
      ast(ast) {

  Sema::GetInstance()->add_func(ast);

  alert;
  for (auto&& arg : ast->arguments) {
    this->add_arg(&arg);
  }

  this->block = new BlockScope(ast->block);
}

} // namespace fire::semantics_checker