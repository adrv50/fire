#pragma once

#include <cassert>
#include <optional>
#include <tuple>
#include <list>

#include "Utils.h"
#include "Error.h"

#include "AST.h"
#include "ASTWalker.h"
#include "Builtin.h"
#include "ScopeContext.h"

namespace fire::semantics_checker {

struct SemaContext;
struct SemaFunctionNameContext;

using namespace AST;
using TypeVec = vector<TypeInfo>;

class Sema;

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
  LocalVar* lvar = nullptr; // if variable

  ASTVec<Function> functions;
  vector<builtins::Function const*> builtin_funcs;

  ASTPtr<Enum> ast_enum = nullptr; // NameType::Enum
  int enumerator_index = 0;        // NameType::Enumerator

  ASTPtr<Class> ast_class = nullptr;

  ASTPtr<AST::Block> ast_namespace = nullptr;

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
    None,
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

struct SemaFunctionNameContext {
  ASTPtr<AST::CallFunc> CF;
  ASTPtr<AST::Signature> Sig;

  TypeVec const* ArgTypes = nullptr;
  TypeInfo const* ExpectedResultType = nullptr;

  bool MustDecideOneCandidate = true;

  bool IsValid() {
    return CF || Sig;
  }

  size_t GetArgumentsCount() {
    return ArgTypes ? ArgTypes->size() : 0;
  }
};

struct SemaContext {

  FunctionScope* original_template_func = nullptr;

  bool create_new_scope = false;

  SemaFunctionNameContext FuncName;

  bool InCallFunc() {
    return FuncName.CF != nullptr;
  }

  bool InOverloadResolGuide() {
    return FuncName.Sig != nullptr;
  }

  static SemaContext NullCtx;
};

struct TemplateInstantiationContext {

  enum ResultTypes {
    TI_NotTryied,

    TI_Succeed,

    TI_TooManyParameters,

    TI_CannotDeductType,

    TI_Arg_TypeMismatch,

    TI_Arg_ParamError, // in formal define

    TI_Arg_TooFew,

    TI_Arg_TooMany,
  };

  struct ParameterInfo {
    string Name;
    TypeInfo Type;

    Token Tok;

    bool IsAssignmented = false;

    ASTPtr<AST::Identifier> TemplateArg = nullptr;

    Vec<ParameterInfo> Params;

    bool IsTypeDeductionCompleted();

    ParameterInfo* GetNotTypeDeducted();

    ParameterInfo(string const& Name, TypeInfo const& T = {});
    ParameterInfo(Token const& token);
    ParameterInfo(AST::Templatable::ParameterName const& P);
  };

  struct InstantiationResult {
    ResultTypes Type = TI_NotTryied;

    ParameterInfo* ErrParam = nullptr;

    TypeInfo Given;

    ASTPointer ErrArgument = nullptr; // or type-ast

    size_t ErrArgIndex = 0;
  };

  Sema& S;

  IdentifierInfo* ParamsII;

  ASTPtr<AST::Templatable> Item; // Templated (original)

  InstantiationResult TryiedResult;

  Vec<ParameterInfo> Params;

  bool Failed = false;

  ASTPointer RequestedBy;

  TemplateInstantiationContext* _Previous = nullptr;

  Error CreateError();

  ParameterInfo* _GetParam_in(Vec<ParameterInfo>& vp, string const& name);

  ParameterInfo* GetParam(string const& name);

  size_t GetAllParameters(Vec<ParameterInfo*>& Opv, ParameterInfo& Iv);

  bool CheckParameterMatchings(ParameterInfo& P);

  bool CheckParameterMatchingToTypeDecl(ParameterInfo& P, TypeInfo const& ArgType,
                                        ASTPtr<AST::TypeName> T);

  bool AssignmentTypeToParam(ParameterInfo* P, ASTPtr<AST::TypeName> TypeAST,
                             ASTPointer Arg, TypeInfo const& type);

  ASTPtr<AST::Function> TryInstantiate_Of_Function(SemaFunctionNameContext* Ctx);

  ASTPtr<AST::Class> TryInstantiate_Of_Class();

  TemplateInstantiationContext(Sema& S, IdentifierInfo* ParamsII,
                               ASTPtr<AST::Templatable> Item);
};

class Sema {

  friend struct BlockScope;
  friend struct FunctionScope;

  friend struct TemplateInstantiationContext;

  friend struct SemaContext;
  friend struct SemaFunctionNameContext;

  ArgumentCheckResult check_function_call_parameters(ASTVector args, bool isVariableArg,
                                                     TypeVec const& formal,
                                                     TypeVec const& actual,
                                                     bool ignore_mismatch);

  LocalVar* _find_variable(string const& name);
  ASTVec<Function> _find_func(string const& name);
  ASTPtr<Enum> _find_enum(string const& name);
  ASTPtr<Class> _find_class(string const& name);
  ASTPtr<Block> _find_namespace(string const& name);

  ASTPointer context_reverse_search(std::function<bool(ASTPointer)> func);

  NameFindResult find_name(string const& name, bool const only_cur_scope = false);

  IdentifierInfo get_identifier_info(ASTPtr<AST::Identifier> ast,
                                     bool only_cur_scope = false);

  IdentifierInfo get_identifier_info(ASTPtr<AST::ScopeResol> ast);

  IdentifierInfo get_identifier_info(ASTPointer ast) {
    if (ast->kind == ASTKind::ScopeResol)
      return this->get_identifier_info(ASTCast<AST::ScopeResol>(ast));

    return this->get_identifier_info(Sema::GetID(ast));
  }

  // -----------------
  // -----------------
  size_t GetMatchedFunctions(ASTVec<AST::Function>& Matched,
                             ASTVec<AST::Function> const& Candidates,
                             IdentifierInfo* ParamsII, SemaContext* Ctx,
                             bool ThrowError = false);

public:
  Sema(ASTPtr<AST::Block> prg);
  ~Sema();

  void check_full();

  void check(ASTPointer ast, SemaContext Ctx = SemaContext::NullCtx);

  TypeInfo eval_type(ASTPointer ast, SemaContext Ctx = SemaContext::NullCtx);

  TypeInfo eval_type_name(ASTPtr<AST::TypeName> ast);

  TypeInfo EvalExpr(ASTPtr<AST::Expr> ast);

  bool IsWritable(ASTPointer ast);

  static Sema* GetInstance();

private:
  struct TemplateInstantiatedRecord {
    ASTPtr<AST::Templatable> Instantiated;
    std::list<ScopeContext*> Scope;
  };

  Vec<TemplateInstantiatedRecord> InstantiatedRecords;

  ASTPtr<Block> root;

  BlockScope* _scope_context = nullptr;
  std::list<ScopeContext*> _scope_history;

  std::list<ScopeContext*>& GetHistory() {
    return _scope_history;
  }

  FunctionScope* cur_function = nullptr;

  ScopeContext* GetRootScope();

  ScopeContext*& GetCurScope();
  ScopeContext* GetScopeOf(ASTPointer ast);

  ScopeContext* EnterScope(ASTPointer ast);
  ScopeContext* EnterScope(ScopeContext* ctx);

  void LeaveScope();

  void ClearScopeHistory() {
    this->GetHistory().clear();
  }

  void SaveScopeLocation();
  void RestoreScopeLocation();

  void BackToDepth(int depth);
  bool BackTo(ScopeContext* ctx);

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

  std::vector<IdentifierInfo> _identifier_info_keep;

  // std::vector<std::pair<ASTPtr<AST::Identifier>, TypeInfo>> id_result_keep;

  IdentifierInfo* get_keeped_id_info(ASTPtr<AST::Identifier> id) {
    for (auto&& info : _identifier_info_keep)
      if (info.ast == id)
        return &info;

    return nullptr;
  }

  IdentifierInfo& keep_id(IdentifierInfo info) {
    return _identifier_info_keep.emplace_back(std::move(info));
  }

  static ASTPtr<AST::TypeName> CreateTypeNameAST_From_TypeInfo(TypeInfo const& type) {
    Token tok;

    if (type.IsPrimitiveType()) {
      tok.str = type.GetSV();
    }
    else if (type.kind == TypeKind::Function) {
      tok.str = "function";
    }

    auto ast = AST::TypeName::New(tok);

    for (auto&& p : type.params) {
      ast->type_params.emplace_back(CreateTypeNameAST_From_TypeInfo(p));
    }

    ast->is_const = type.is_const;

    return ast;
  }
};

} // namespace fire::semantics_checker