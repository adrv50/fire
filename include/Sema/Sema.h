#pragma once

#include <cassert>
#include <optional>
#include <tuple>

#include "Utils.h"

#include "AST.h"
#include "Error.h"

#include "ScopeContext.h"

namespace fire::semantics_checker {

using namespace AST;
using TypeVec = vector<TypeInfo>;

class Sema;

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

    // vector<TypeInfo> arg_types;

    bool is_type_deducted = false; // of result_type
    TypeInfo result_type;

    vector<ASTPtr<AST::Statement>>
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
  vector<ScopeContext*> _scope_history;

  ScopeContext*& GetCurScope();
  ScopeContext* GetScopeOf(ASTPointer ast);

  ScopeContext* EnterScope(ASTPointer ast);
  void LeaveScope(ASTPointer ast);

  ScopeContext* EnterScope(ScopeContext* ctx);
  void LeaveScope(ScopeContext* ctx);

  void SaveScopeInfo();
  void RestoreScopeInfo();

  void BackToDepth(int depth);

  int GetScopesOfDepth(vector<ScopeContext*>& out, ScopeContext* scope, int depth);

  vector<SemaFunction> functions;

  ASTVec<Enum> enums;
  ASTVec<Class> classes;

  SemaFunction& add_func(ASTPtr<Function> f) {
    return this->functions.emplace_back(f);
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