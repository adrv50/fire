#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

namespace fire::semantics_checker {

ScopeContext::LocalVar::LocalVar(ASTPtr<AST::VarDef> vardef) {

  this->name = vardef->GetName();
  this->decl = vardef;

  // if (vardef->type) {
  //   this->deducted_type = Sema::GetInstance()->EvalType(vardef->type);
  //   this->is_type_deducted = true;
  // }

  // if (vardef->init) {
  //   this->deducted_type = Sema::GetInstance()->EvalType(vardef->init);
  //   this->is_type_deducted = true;
  // }
}

ScopeContext::LocalVar::LocalVar(ASTPtr<AST::Argument> arg) {
  // Don't use this ctor if arg is template.

  this->name = arg->GetName();
  this->arg = arg;

  // this->deducted_type = Sema::GetInstance()->EvalType(arg->type);
  // this->is_type_deducted = true;
}

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

    return func->block == scope ||
           (recursive && func->block->Contains(scope, recursive));
  }

  default:
    todo_impl;
  }

  return false;
}

ASTPointer ScopeContext::GetAST() const {
  return nullptr;
}

ScopeContext::LocalVar* ScopeContext::find_var(string const&) {
  return nullptr;
}

ScopeContext* ScopeContext::find_child_scope(ASTPointer) {
  return nullptr;
}

ScopeContext* ScopeContext::find_child_scope(ScopeContext*) {
  return nullptr;
}

vector<ScopeContext*> ScopeContext::find_name(string const&) {
  return {};
}

// ------------------------------------
//  BlockScope

BlockScope::BlockScope(ASTPtr<AST::Block> ast)
    : ScopeContext(SC_Block),
      ast(ast) {

  // Sema::GetInstance()->_scope_context = this;

  for (auto&& e : ast->list) {
    AST::walk_ast(e, [this](ASTWalkerLocation loc, ASTPointer x) {
      if (loc != AW_Begin)
        return;

      switch (x->kind) {
      case ASTKind::Block:
        this->AddScope(new BlockScope(ASTCast<AST::Block>(x)));
        break;

      case ASTKind::Function:
        this->AddScope(new FunctionScope(ASTCast<AST::Function>(x)));
        break;

      case ASTKind::Vardef:
        this->add_var(ASTCast<AST::VarDef>(x));
        break;
      }
    });
  }
}

BlockScope::~BlockScope() {
  for (auto&& c : this->child_scopes)
    delete c;
}

ScopeContext::LocalVar& BlockScope::add_var(ASTPtr<AST::VarDef> def) {
  LocalVar* pvar = this->find_var(def->GetName());

  if (!pvar)
    pvar = &this->variables.emplace_back(def);

  pvar->depth = this->depth;
  pvar->index = def->index = this->variables.size() - 1;

  this->ast->stack_size++;

  return *pvar;
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

vector<ScopeContext*> BlockScope::find_name(string const& name) {
  vector<ScopeContext*> vec;

  for (auto&& c : this->child_scopes) {
    if (c->IsNamedAs(name)) {
      vec.emplace_back(c);
    }
  }

  return vec;
}

// ------------------------------------
//  FunctionScope

FunctionScope::FunctionScope(ASTPtr<AST::Function> ast)
    : ScopeContext(SC_Func),
      ast(ast) {

  auto S = Sema::GetInstance();

  for (auto&& [_key, _val] : S->function_scope_map) {
    if (_key == ast) {
      todo_impl; // why again?
    }
  }

  // auto f = Sema::SemaFunction(ast);
  // f.scope = this;

  S->function_scope_map.emplace_back(ast, this);

  for (auto&& arg : ast->arguments) {
    this->add_arg(arg);
  }

  this->block = new BlockScope(ast->block);
  this->block->depth = this->depth + 1;
}

FunctionScope::~FunctionScope() {
  delete this->block;

  for (auto&& s : this->instantiated)
    delete s;
}

ScopeContext::LocalVar& FunctionScope::add_arg(ASTPtr<AST::Argument> def) {
  auto& arg = this->arguments.emplace_back(def->GetName());

  arg.arg = def;
  arg.is_argument = true;

  arg.depth = this->depth;
  arg.index = this->arguments.size() - 1;

  if (!this->is_templated()) {
    arg.deducted_type = Sema::GetInstance()->EvalType(def->type);
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
    if (auto x = I->find_child_scope(ast); x)
      return x;
  }

  return this->block->find_child_scope(ast);
}

ScopeContext* FunctionScope::find_child_scope(ScopeContext* ctx) {
  if (this == ctx)
    return this;

  for (auto&& I : this->instantiated) {
    if (auto x = I->find_child_scope(ctx); x)
      return x;
  }

  return this->block->find_child_scope(ctx);
}

vector<ScopeContext*> FunctionScope::find_name(string const& name) {
  return this->block->find_name(name);
}

// ------------------------------------

} // namespace fire::semantics_checker