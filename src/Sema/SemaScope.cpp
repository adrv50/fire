#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

namespace fire::semantics_checker {

// ------------------------------------
//  ScopeContext ( base-struct )

ScopeContext* ScopeContext::find_child_scope(ASTPointer ast) {
  return nullptr;
}

ASTPointer ScopeContext::GetAST() const {
  return nullptr;
}

ScopeContext::LocalVar* ScopeContext::find_var(string const& name) {
  return nullptr;
}

// ------------------------------------
//  BlockScope

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

ASTPointer BlockScope::GetAST() const {
  return this->ast;
}

ScopeContext::LocalVar* BlockScope::find_var(string const& name) {
  for (auto&& var : this->variables) {
    if (var.name == name)
      return &var;
  }

  return nullptr;
}

ScopeContext* BlockScope::find_child_scope(ASTPointer ast) {
  if (this->ast == ast)
    return this;

  for (auto&& c : this->child_scopes) {
    if (auto s = c->find_child_scope(ast); s)
      return s;
  }

  return nullptr;
}

// ------------------------------------
//  FunctionScope

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

ASTPointer FunctionScope::GetAST() const {
  return this->ast;
}

ScopeContext::LocalVar* FunctionScope::find_var(string const& name) {
  for (auto&& arg : this->arguments) {
    if (arg.name == name)
      return &arg;
  }

  return nullptr;
}

ScopeContext* FunctionScope::find_child_scope(ASTPointer ast) {
  if (this->ast == ast)
    return this;

  return this->block->find_child_scope(ast);
}

// ------------------------------------

} // namespace fire::semantics_checker