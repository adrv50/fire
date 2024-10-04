#pragma once

#include <cassert>
#include <optional>
#include <tuple>
#include <list>

#include "Utils.h"
#include "Error.h"

#include "AST.h"
#include "Builtin.h"
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

    // variable
    Var,

    Func,
    BuiltinFunc,

    MemberFunc,

    Enum,
    Enumerator,

    Class,
    Namespace,

    TypeName, // builtin type name
  };

  struct NameFindResult {
    NameType type = NameType::Unknown;

    string name;
    ScopeContext::LocalVar* lvar = nullptr; // if variable

    ASTVec<Function> functions;
    vector<builtins::Function const*> builtin_funcs;

    ASTPtr<Enum> ast_enum = nullptr; // NameType::Enum
    int enumerator_index = 0;        // NameType::Enumerator

    ASTPtr<Class> ast_class;

    TypeKind kind = TypeKind::Unknown;

    bool is_type_name() const {
      switch (this->type) {
      case NameType::Enum:
      case NameType::Class:
      case NameType::TypeName:
        return true;
      }

      return false;
    }
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

    vector<IdentifierInfo> template_args;

    NameFindResult result;

    string to_string() const;
  };

  // struct SemaFunction {
  //   ASTPtr<Function> func;

  //   FunctionScope* scope = nullptr;

  //   // vector<TypeInfo> arg_types;

  //   bool is_type_deducted = false; // of result_type
  //   TypeInfo result_type;

  //   vector<ASTPtr<AST::Statement>>
  //       return_stmt_list; // use to check return-type specification.
  //                         // (append only that contains a expression)

  //   SemaFunction(ASTPtr<Function> func);
  // };

  ArgumentCheckResult check_function_call_parameters(ASTVector args, bool isVariableArg,
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

  IdentifierInfo get_identifier_info(ASTPointer ast) {
    if (ast->kind == ASTKind::ScopeResol)
      return this->get_identifier_info(ASTCast<AST::ScopeResol>(ast));

    return this->get_identifier_info(Sema::GetID(ast));
  }

public:
  struct ScopeLocation {
    ScopeContext* Current;
    vector<ScopeContext*> History;
  };

  Sema(ASTPtr<AST::Block> prg);
  ~Sema();

  void check_full();

  void check(ASTPointer ast);

  TypeInfo eval_type(ASTPointer ast);

  TypeInfo eval_type_name(ASTPtr<AST::TypeName> ast);

  TypeInfo EvalExpr(ASTPtr<AST::Expr> ast);

  bool IsWritable(ASTPointer ast);

  static Sema* GetInstance();

private:
  struct InstantiateTask {
    ASTPtr<AST::Function>
  };

  vector<InstantiateTask> ins_requests;

  InstantiateRequest* find_request_of_func(ASTPtr<AST::Function> func, TypeInfo ret_type,
                                           TypeVec args); // args = actual

  ASTPtr<AST::Function>
  instantiate_template_func(ASTPtr<AST::Function> func, ASTPointer requested,
                            ASTPtr<AST::Identifier> id, ASTVector args,
                            TypeVec const& arg_types, bool ignore_args);

  struct FunctionSignature {
    ASTVector template_args;
    ASTVector args;
    TypeVec arg_types;
  };

  i64 resolution_overload(ASTVec<AST::Function>& out,
                          ASTVec<AST::Function> const& candidates,
                          FunctionSignature const& sig);

  int _construct_scope_context(ScopeContext& S, ASTPointer ast);

  ASTPtr<Block> root;

  BlockScope* _scope_context = nullptr;

  // ScopeContext* _cur_scope = _scope_context;
  // vector<ScopeContext*> _scope_history;

  ScopeLocation _location;

  FunctionScope* cur_function = nullptr;

  ScopeContext* GetRootScope();

  ScopeContext*& GetCurScope();
  ScopeContext* GetScopeOf(ASTPointer ast);

  ScopeContext* EnterScope(ASTPointer ast);
  ScopeContext* EnterScope(ScopeContext* ctx);

  // void LeaveScope(ASTPointer ast);
  // void LeaveScope(ScopeContext* ctx);
  void LeaveScope();

  struct InstantiationScope {
    vector<std::pair<string, TypeInfo>> arg_types;

    void add_name(string const& name, TypeInfo const& type) {
      this->arg_types.emplace_back(name, type);
    }

    TypeInfo* find_name(string const& name) {
      for (auto&& [n, t] : this->arg_types)
        if (n == name)
          return &t;

      return nullptr;
    }
  };

  std::list<InstantiationScope> inst_scope;

  InstantiationScope& enter_instantiation_scope() {
    auto& scope = this->inst_scope.emplace_front();

    return scope;
  }

  void leave_instantiation_scope() {
    this->inst_scope.pop_front();
  }

  TypeInfo* find_template_parameter_name(string const& name) {
    for (auto&& inst : this->inst_scope) {
      if (auto p = inst.find_name(name); p)
        return p;
    }

    return nullptr;
  }

  void SaveScopeLocation();
  void RestoreScopeLocation();
  ScopeLocation GetScopeLoc();

  void BackToDepth(int depth);

  int GetScopesOfDepth(vector<ScopeContext*>& out, ScopeContext* scope, int depth);

  TypeInfo ExpectType(TypeInfo const& type, ASTPointer ast);
  TypeInfo* GetExpectedType();

  bool IsExpected(TypeKind kind);

  vector<TypeInfo> _expected;

  vector<std::pair<ASTPtr<AST::Function>, FunctionScope*>> function_scope_map;

  ASTVec<Enum> enums;
  ASTVec<Class> classes;

  ASTPtr<Enum>& add_enum(ASTPtr<Enum> e) {
    return this->enums.emplace_back(e);
  }

  ASTPtr<Class>& add_class(ASTPtr<Class> c) {
    return this->classes.emplace_back(c);
  }

  static ASTPtr<AST::Identifier> GetID(ASTPointer ast) {
    if (ast->_constructed_as == ASTKind::MemberAccess)
      return GetID(ast->as_expr()->rhs);

    if (ast->_constructed_as == ASTKind::ScopeResol)
      return ASTCast<AST::ScopeResol>(ast)->GetLastID();

    assert(ast->_constructed_as == ASTKind::Identifier);
    return ASTCast<AST::Identifier>(ast);
  }

  TypeInfo make_functor_type(ASTPtr<AST::Function> ast);
  TypeInfo make_functor_type(builtins::Function const* builtin);
};

} // namespace fire::semantics_checker