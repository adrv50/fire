#pragma once

#include <cassert>
#include <optional>
#include <tuple>

#include "AST.h"

namespace fire::semantics_checker {

using namespace AST;

using TypeVec = vector<TypeInfo>;

class Sema;

// ------------------------------------
//  ScopeContext ( base-struct )

struct ScopeContext {

  enum Types {
    SC_Block,
    SC_Func,
    SC_Enum,
    SC_Class,
  };

  struct LocalVar {
    string name;

    TypeInfo deducted_type;
    bool is_type_deducted = false;

    bool is_argument = false;
    ASTPtr<AST::VarDef> decl = nullptr;
    ASTPtr<AST::Argument> arg = nullptr;

    int depth = 0;
    int index = 0;

    LocalVar(string name)
        : name(name) {
    }
  };

  Types type;

  int depth = 0;

  bool Contains(ScopeContext* scope, bool recursive = false) const;

  virtual ASTPointer GetAST() const;

  virtual LocalVar* find_var(string const& name);

  virtual ScopeContext* find_child_scope(ASTPointer ast);
  virtual ScopeContext* find_child_scope(ScopeContext* ctx);

  virtual ~ScopeContext() = default;

protected:
  ScopeContext(Types type)
      : type(type) {
  }
};

// ------------------------------------
//  BlockScope

struct BlockScope : ScopeContext {
  ASTPtr<AST::Block> ast;

  vector<LocalVar> variables;

  vector<ScopeContext*> child_scopes;

  ScopeContext*& AddScope(ScopeContext* s) {
    s->depth = this->depth + 1;

    return this->child_scopes.emplace_back(s);
  }

  LocalVar& add_var(ASTPtr<AST::VarDef> def) {
    auto& var = this->variables.emplace_back(def->GetName());

    (void)var;
    todo_impl;

    return var;
  }

  ASTPointer GetAST() const override;

  LocalVar* find_var(string const& name) override;

  ScopeContext* find_child_scope(ASTPointer ast) override;
  ScopeContext* find_child_scope(ScopeContext* ctx) override;

  BlockScope(ASTPtr<AST::Block> ast);
};

// ------------------------------------
//  FunctionScope

struct FunctionScope : ScopeContext {
  ASTPtr<AST::Function> ast = nullptr;

  vector<LocalVar> arguments;

  BlockScope* block = nullptr;

  vector<FunctionScope*> instantiated;

  FunctionScope*& AppendInstantiated(ASTPtr<AST::Function> fn) {
    auto& fs = this->instantiated.emplace_back(new FunctionScope(fn));

    fs->depth = this->depth;

    return fs;
  }

  bool is_templated() const {
    return ast->is_templated;
  }

  LocalVar& add_arg(ASTPtr<AST::Argument> def);

  ASTPointer GetAST() const override;

  LocalVar* find_var(string const& name) override;
  ScopeContext* find_child_scope(ScopeContext* ctx) override;

  ScopeContext* find_child_scope(ASTPointer ast) override;

  FunctionScope(ASTPtr<AST::Function> ast);
};

} // namespace fire::semantics_checker