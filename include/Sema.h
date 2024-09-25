#pragma once

#include <cassert>
#include <optional>
#include <tuple>

#include "Utils.h"

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
    AST::Function::Argument* arg = nullptr;

    int depth = 0;
    int index = 0;

    LocalVar(string name)
        : name(name) {
    }
  };

  Types type;

  virtual ScopeContext* find_child_scope(ASTPointer ast) const;

  virtual ~ScopeContext() = default;

protected:
  ScopeContext(Types type)
      : type(type) {
  }
};

struct BlockScope : ScopeContext {
  ASTPtr<AST::Block> ast;

  vector<LocalVar> variables;

  vector<ScopeContext*> child_scopes;

  ScopeContext*& AddScope(ScopeContext* s) {
    return this->child_scopes.emplace_back(s);
  }

  LocalVar& add_var(ASTPtr<AST::VarDef> def) {
    auto& var = this->variables.emplace_back(def->GetName());

    (void)var;
    todo_impl;

    return var;
  }

  ScopeContext* find_child_scope(ASTPointer ast) const override;

  BlockScope(ASTPtr<AST::Block> ast);
};

struct FunctionScope : ScopeContext {
  ASTPtr<AST::Function> ast = nullptr;

  vector<LocalVar> arguments;

  BlockScope* block = nullptr;

  vector<FunctionScope*> instantiated_templates;

  bool is_templated() const {
    return ast->is_templated;
  }

  LocalVar& add_arg(AST::Function::Argument* def);

  ScopeContext* find_child_scope(ASTPointer ast) const override;

  FunctionScope(ASTPtr<AST::Function> ast);
};

class Sema {

  friend struct BlockScope;
  friend struct FunctionScope;

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

    FunctionScope* scope = nullptr;

    vec<TypeInfo> arg_types;

    TypeInfo result_type;

    vec<ASTPtr<AST::Statement>>
        return_stmt_list; // use to check return-type specification.
                          // (append only that contains a expression)

    SemaFunction(ASTPtr<Function> func);
  };

  struct TemplateInstantiator {
    ASTPointer ast_cloned;

    vector<std::pair<string, TypeInfo>> name_and_type;

    void do_replace();

    TemplateInstantiator(ASTPtr<AST::Templatable> templatable);
  };

  ArgumentCheckResult check_function_call_parameters(ASTPtr<CallFunc> call,
                                                     bool isVariableArg,
                                                     TypeVec const& formal,
                                                     TypeVec const& actual,
                                                     bool ignore_mismatch);

  ScopeContext::LocalVar* _find_variable(string const& name);
  ASTVec<Function> _find_func(string const& name);
  ASTPtr<Enum> _find_enum(string const& name);
  ASTPtr<Class> _find_class(string const& name);

  NameFindResult find_name(string const& name);

  IdentifierInfo get_identifier_info(ASTPtr<AST::Identifier> ast);
  IdentifierInfo get_identifier_info(ASTPtr<AST::ScopeResol> ast);

public:
  Sema(ASTPtr<AST::Block> prg);
  ~Sema();

  void check_full();

  void check(ASTPointer ast);

  TypeInfo evaltype(ASTPointer ast);

  static Sema* GetInstance();

private:
  int _construct_scope_context(ScopeContext& S, ASTPointer ast);

  ASTPtr<Block> root;

  BlockScope* _scope_context = nullptr;
  ScopeContext* _cur_scope = _scope_context;

  ScopeContext* GetCurScope();
  ScopeContext* EnterScope(ASTPointer ast);
  void LeaveScope(ASTPointer ast);

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