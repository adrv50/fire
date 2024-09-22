#pragma once

#include <cassert>
#include <optional>
#include <tuple>

#include "AST.h"
#include "Error.h"

namespace fire::semantics_checker {

template <class T>
using vec = std::vector<T>;

template <class T>
using ASTPtr = ASTPtr<T>;

template <class T>
using WeakVec = vec<std::weak_ptr<T>>;

using TypeVec = vec<TypeInfo>;

using namespace AST;

class Sema;

struct ScopeContext {
  struct LocalVar {
    string name;
    TypeInfo type;

    ASTPtr<VarDef> decl = nullptr;

    bool is_type_deducted = false;

    int index = 0;

    LocalVar(string const& name, TypeInfo type = {})
        : name(name),
          type(std::move(type)) {
    }
  };

  Sema& S;

  ASTPtr<Block> ast;
  ScopeContext* parent = nullptr;

  vec<LocalVar> variables;

  vec<ScopeContext*> child_scopes;

  LocalVar* get_var(string const& name);

  //
  // result:
  //   [0]      = if found same name, true
  //   [1], [2] = index or pointer to var
  //
  std::tuple<bool, int, LocalVar*> define_var(ASTPtr<VarDef> ast);

  ScopeContext(Sema& S, ASTPtr<Block> ast, ScopeContext* parent)
      : S(S),
        ast(ast),
        parent(parent) {
  }

  ~ScopeContext() {
    for (auto&& _c : this->child_scopes)
      delete _c;
  }
};

struct ScopeContextPointer {

  ScopeContext* get() {
    return this->_ptr;
  }

  ScopeContext* enter(ASTPtr<Block> block) {
    for (auto&& c : this->_ptr->child_scopes) {
      if (c->ast == block)
        return this->_ptr = c;
    }

    return nullptr;
  }

  ScopeContext* leave() {
    return this->_ptr = this->_ptr->parent;
  }

  ScopeContextPointer(ScopeContext* p)
      : _ptr(p) {
  }

private:
  ScopeContext* _ptr;
};

class Sema {

  enum class NameType {
    Unknown,
    Var,
    Func,
    Enum,
    Enumerator,
    Class,
    Namespace,
  };

  struct NameFindResult {
    NameType type = NameType::Unknown;

    string name;
    ScopeContext::LocalVar* lvar = nullptr; // if variable

    ASTVec<Function> functions;

    ASTPtr<Enum> ast_enum = nullptr; // NameType::Enum
    int enumerator_index = 0;        // NameType::Enumerator

    ASTPtr<Class> ast_class;
  };

  struct ArgumentCheckResult {
    enum Result {
      Ok,
      TooFewArguments,
      TooManyArguments,
      TypeMismatch,
    };

    Result result;
    int index;

    ArgumentCheckResult(Result r, int i = 0)
        : result(r),
          index(i) {
    }
  };

  struct IdentifierInfo {
    ASTPtr<AST::Identifier> ast;

    TypeVec id_params;

    NameFindResult result;

    string to_string() const;
  };

  struct SemaFunction {
    ASTPtr<Function> func;

    vec<TypeInfo> arg_types;

    TypeInfo result_type;

    vec<ASTPtr<AST::Statement>>
        return_stmt_list; // use to check return-type specification.

    SemaFunction(ASTPtr<Function> func);
  };

  ArgumentCheckResult
  check_function_call_parameters(ASTPtr<CallFunc> call, bool isVariableArg,
                                 TypeVec const& formal,
                                 TypeVec const& actual);

  ScopeContext::LocalVar* _find_variable(string const& name);
  ASTVec<Function> _find_func(string const& name);
  ASTPtr<Enum> _find_enum(string const& name);
  ASTPtr<Class> _find_class(string const& name);

  NameFindResult find_name(string const& name);

  IdentifierInfo get_identifier_info(ASTPtr<AST::Identifier> ast);
  IdentifierInfo get_identifier_info(ASTPtr<AST::ScopeResol> ast);

public:
  Sema(ASTPtr<AST::Block> prg);

  void check_full();

  void check(ASTPointer ast);

  TypeInfo evaltype(ASTPointer ast);

private:
  int _construct_scope_context(ScopeContext& S, ASTPointer ast);

  ASTPtr<Block> root;
  ScopeContext _scope_context;

  ScopeContextPointer scope_ptr = &_scope_context;

  vec<SemaFunction> functions;

  ASTVec<Enum> enums;
  ASTVec<Class> classes;

  SemaFunction& add_func(ASTPtr<Function> f) {
    auto& x = this->functions.emplace_back(f);

    for (auto&& arg : f->arguments)
      x.arg_types.emplace_back(this->evaltype(arg.type));

    x.result_type = this->evaltype(f->return_type);

    return x;
  }

  SemaFunction* get_func(ASTPtr<Function> f) {
    for (auto&& sf : this->functions)
      if (sf.func == f)
        return &sf;

    return nullptr;
  }

  ASTPtr<Enum>& add_enum(ASTPtr<Enum> e) {
    return this->enums.emplace_back(e);
  }

  ASTPtr<Class>& add_class(ASTPtr<Class> c) {
    return this->classes.emplace_back(c);
  }
};

} // namespace fire::semantics_checker