#pragma once

#include <cassert>
#include <optional>
#include <tuple>

#include "AST.h"

namespace fire::semantics_checker {

using namespace AST;

using TypeVec = vector<TypeInfo>;

class Sema;

struct LocalVar {
  string name;

  TypeInfo deducted_type;
  bool is_type_deducted = false;

  bool is_argument = false;
  ASTPtr<AST::VarDef> decl = nullptr;
  ASTPtr<AST::Argument> arg = nullptr;

  int depth = 0;
  int index = 0;

  int index_add = 0;

  LocalVar(string const& name = "")
      : name(name) {
  }

  LocalVar(ASTPtr<AST::VarDef> vardef);
  LocalVar(ASTPtr<AST::Argument> arg);
};

// ------------------------------------
//  ScopeContext ( base-struct )

struct ScopeContext {

  enum Types {
    SC_Block,
    SC_Func,
    SC_Enum,
    SC_Class,
    SC_Namespace,
  };

  Types type;

  int depth = 0;

  bool is_block;

  ScopeContext* _owner = nullptr;

  bool Contains(ScopeContext* scope, bool recursive = false) const;

  virtual bool IsNamedAs(string const&) const {
    return false;
  }

  virtual ASTPointer GetAST() const;

  virtual LocalVar* find_var(string_view const& name);

  virtual ScopeContext* find_child_scope(ASTPointer ast);
  virtual ScopeContext* find_child_scope(ScopeContext* ctx);

  virtual bool contains(ScopeContext*) {
    return false;
  }

  // find a named scope
  virtual vector<ScopeContext*> find_name(string const& name);

  virtual ~ScopeContext() = default;

  virtual std::string to_string() const = 0;

protected:
  ScopeContext(Types type)
      : type(type),
        is_block(type == SC_Block) {
  }
};

// ------------------------------------
//  BlockScope

struct BlockScope : ScopeContext {
  ASTPtr<AST::Block> ast;

  vector<LocalVar> variables;

  size_t child_var_count = 0;

  vector<ScopeContext*> child_scopes;

  ScopeContext*& AddScope(ScopeContext* s);

  LocalVar& add_var(ASTPtr<AST::VarDef> def);

  ASTPointer GetAST() const override;

  LocalVar* find_var(string_view const& name) override;

  ScopeContext* find_child_scope(ASTPointer ast) override;
  ScopeContext* find_child_scope(ScopeContext* ctx) override;

  bool contains(ScopeContext* ctx) override {
    for (auto&& c : this->child_scopes)
      if (c == ctx)
        return true;

    return false;
  }

  vector<ScopeContext*> find_name(string const& name) override;

  std::string to_string() const override;

  BlockScope(int depth, ASTPtr<AST::Block> ast, int index_add = 0);
  ~BlockScope();
};

// ------------------------------------
//  FunctionScope

struct FunctionScope : ScopeContext {
  ASTPtr<AST::Function> ast = nullptr;

  vector<LocalVar> arguments;

  BlockScope* block = nullptr;

  TypeInfo result_type;

  ASTVec<AST::Statement> return_stmt_list;

  Vec<FunctionScope*> InstantiatedTemplateFunctions;

  bool IsTemplatedFunc() const {
    return ast->IsTemplated;
  }

  LocalVar& add_arg(ASTPtr<AST::Argument> def);

  bool IsNamedAs(string const& name) const override {
    return this->ast->GetName() == name;
  }

  ASTPointer GetAST() const override;

  LocalVar* find_var(string_view const& name) override;

  ScopeContext* find_child_scope(ScopeContext* ctx) override;
  ScopeContext* find_child_scope(ASTPointer ast) override;

  bool contains(ScopeContext* ctx) override {
    return this->block == ctx;
  }

  vector<ScopeContext*> find_name(string const& name) override;

  std::string to_string() const override;

  FunctionScope(int depth, ASTPtr<AST::Function> ast);
  ~FunctionScope();
};

// ------------------------------------
//  NamespaceScope

struct NamespaceScope : BlockScope {
  string name;

  ASTVec<AST::Block> _ast;

  ScopeContext* find_child_scope(ASTPointer ast) override {
    for (auto&& x : this->_ast) {
      if (x == ast)
        return this;
    }

    return BlockScope::find_child_scope(ast);
  }

  NamespaceScope(int depth, ASTPtr<AST::Block> ast, int index_add);
  ~NamespaceScope();
};

} // namespace fire::semantics_checker