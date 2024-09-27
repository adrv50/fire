#pragma once

#include <cassert>
#include <optional>
#include <tuple>

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
    vector<builtins::Function const*> builtin_funcs;

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
  struct ScopeLocation {
    ScopeContext* Current;
    vector<ScopeContext*> History;
  };

  Sema(ASTPtr<AST::Block> prg);
  ~Sema();

  void check_full();

  void check(ASTPointer ast);

  TypeInfo evaltype(ASTPointer ast);

  static Sema* GetInstance();

private:
  struct InstantiateRequest {

    struct Argument {
      TypeInfo type;

      ASTPtr<AST::TypeName> ast = nullptr; // 明示的に渡された場合

      bool is_deducted = false;
    };

    ASTPointer requested = nullptr;

    ScopeLocation scope_loc;

    IdentifierInfo idinfo;

    // パラメータとして渡された型
    std::map<string, Argument> param_types;

    ASTPtr<AST::Function> original = nullptr;
    ASTPtr<AST::Function> cloned = nullptr;

    TypeInfo result_type; //
    TypeVec arg_types;    // function
  };

  std::vector<InstantiateRequest> ins_requests;

  InstantiateRequest* find_request_of_func(ASTPtr<AST::Function> func, TypeInfo ret_type,
                                           TypeVec args); // args = actual

  ASTPtr<AST::Function> Instantiate(ASTPtr<AST::Function> func,
                                    ASTPtr<AST::CallFunc> call, IdentifierInfo idinfo,
                                    ASTPtr<AST::Identifier> id, TypeVec const& arg_types);

  int _construct_scope_context(ScopeContext& S, ASTPointer ast);

  ASTPtr<Block> root;

  BlockScope* _scope_context = nullptr;

  // ScopeContext* _cur_scope = _scope_context;
  // vector<ScopeContext*> _scope_history;

  ScopeLocation _location;

  ScopeContext* GetRootScope();

  ScopeContext*& GetCurScope();
  ScopeContext* GetScopeOf(ASTPointer ast);

  ScopeContext* EnterScope(ASTPointer ast);
  ScopeContext* EnterScope(ScopeContext* ctx);

  // void LeaveScope(ASTPointer ast);
  // void LeaveScope(ScopeContext* ctx);
  void LeaveScope();

  void SaveScopeLocation();
  void RestoreScopeLocation();
  ScopeLocation GetScopeLoc();

  void BackToDepth(int depth);

  int GetScopesOfDepth(vector<ScopeContext*>& out, ScopeContext* scope, int depth);

  vector<std::pair<ASTPtr<AST::Function>, FunctionScope*>> function_scope_map;

  ASTVec<Enum> enums;
  ASTVec<Class> classes;

  ASTPtr<Enum>& add_enum(ASTPtr<Enum> e) {
    return this->enums.emplace_back(e);
  }

  ASTPtr<Class>& add_class(ASTPtr<Class> c) {
    return this->classes.emplace_back(c);
  }
};

} // namespace fire::semantics_checker