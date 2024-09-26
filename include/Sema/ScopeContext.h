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

    LocalVar(string name = "")
        : name(std::move(name)) {
    }

    LocalVar(ASTPtr<AST::VarDef> vardef);
    LocalVar(ASTPtr<AST::Argument> arg);
  };

  Types type;

  int depth = 0;

  bool Contains(ScopeContext* scope, bool recursive = false) const;

  virtual bool IsNamedAs(string const&) const {
    return false;
  }

  virtual ASTPointer GetAST() const;

  virtual LocalVar* find_var(string const& name);

  virtual ScopeContext* find_child_scope(ASTPointer ast);
  virtual ScopeContext* find_child_scope(ScopeContext* ctx);

  // find a named scope
  virtual vector<ScopeContext*> find_name(string const& name);

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

  LocalVar& add_var(ASTPtr<AST::VarDef> def);

  ASTPointer GetAST() const override;

  LocalVar* find_var(string const& name) override;

  ScopeContext* find_child_scope(ASTPointer ast) override;
  ScopeContext* find_child_scope(ScopeContext* ctx) override;

  vector<ScopeContext*> find_name(string const& name) override;

  BlockScope(ASTPtr<AST::Block> ast);
  ~BlockScope();
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

  bool IsNamedAs(string const& name) const override {
    return this->ast->GetName() == name;
  }

  ASTPointer GetAST() const override;

  LocalVar* find_var(string const& name) override;

  ScopeContext* find_child_scope(ScopeContext* ctx) override;
  ScopeContext* find_child_scope(ASTPointer ast) override;

  vector<ScopeContext*> find_name(string const& name) override;

  FunctionScope(ASTPtr<AST::Function> ast);
  ~FunctionScope();
};

} // namespace fire::semantics_checker