#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

namespace fire::semantics_checker {

// ------------------------------------
//  ScopeContext ( base-struct )

bool ScopeContext::Contains(ScopeContext* scope, bool recursive) const {

  switch (this->type) {
  case SC_Block: {
    auto block = (BlockScope*)scope;

    for (auto&& c : block->child_scopes) {
      if (c == scope)
        return true;

      if (recursive && c->Contains(scope, recursive))
        return true;
    }

    break;
  }

  case SC_Func: {
    auto func = (FunctionScope*)scope;

    return func->block == scope || (recursive && func->block->Contains(scope, recursive));
  }

  default:
    todo_impl;
  }

  return false;
}

ASTPointer ScopeContext::GetAST() const {
  return nullptr;
}

ScopeContext* ScopeContext::find_child_scope(ASTPointer ast) {
  return nullptr;
}

ScopeContext* ScopeContext::find_child_scope(ScopeContext* ctx) {
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
  for (auto&& e : ast->list) {
    switch (e->kind) {
    case ASTKind::Block:
      this->AddScope(new BlockScope(ASTCast<AST::Block>(e)));
      break;

    case ASTKind::Function:
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

ScopeContext* BlockScope::find_child_scope(ScopeContext* ctx) {
  if (this == ctx)
    return this;

  for (auto&& c : this->child_scopes) {
    if (c == ctx)
      return c;
  }

  return nullptr;
}

// ------------------------------------
//  FunctionScope

FunctionScope::FunctionScope(ASTPtr<AST::Function> ast)
    : ScopeContext(SC_Func),
      ast(ast) {

  Sema::GetInstance()->add_func(ast);

  for (auto&& arg : ast->arguments) {
    this->add_arg(arg);
  }

  this->block = new BlockScope(ast->block);
  this->block->depth = this->depth + 1;
}

ScopeContext::LocalVar& FunctionScope::add_arg(ASTPtr<AST::Argument> def) {
  auto& arg = this->arguments.emplace_back(def->GetName());

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

  for (auto&& I : this->instantiated) {
    if (I->GetAST() == ast)
      return I;
  }

  return this->block->find_child_scope(ast);
}

ScopeContext* FunctionScope::find_child_scope(ScopeContext* ctx) {
  if (this == ctx)
    return this;

  for (auto&& I : this->instantiated) {
    if (I == ctx)
      return I;
  }

  return this->block->find_child_scope(ctx);
}

// ------------------------------------

} // namespace fire::semantics_checker