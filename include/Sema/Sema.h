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

struct SemaIdentifierEvalResult;

using TypeVec = vector<TypeInfo>;

class Sema;

enum class NameType {
  Unknown,

  // variable
  Var,

  MemberVar,

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

  bool IsValid() const {
    return CF || Sig;
  }

  size_t GetArgumentsCount() const {
    return ArgTypes ? ArgTypes->size() : 0;
  }
};

struct SemaClassNameContext {

  ASTPtr<AST::Class> Now_Analysing;

  ASTPtr<AST::Class> InheritBaseClass;

  bool PassMemberAnalyze = false;
};

struct SemaMemberReferenceContext {

  bool IsValid = false;

  ASTPtr<AST::Expr> RefExpr;

  ASTPointer Left;
  ASTPointer Right;

  TypeInfo LeftType;

  IdentifierInfo LeftNameII;

  ASTPointer ReferencedItem;
};

struct SemaScopeResolContext {

  // todo
};

struct SemaTypeExpectionContext {

  TypeInfo const* Expected = nullptr;
};

struct SemaContext {

  //
  // 関数呼び出し式において，呼び出し先の関数を一つに決める必要がある場合に使用
  SemaFunctionNameContext* FuncNameCtx;

  //
  // クラス名として参照される式である場合に使用
  // また，クラス内を解析している場合は，そのクラスへのポインタを格納．
  SemaClassNameContext* ClassCtx;

  SemaMemberReferenceContext* MemberRefCtx;

  SemaScopeResolContext* ScopeResolCtx;

  SemaTypeExpectionContext* TypeExpection;

  bool InCallFunc() const {
    return FuncNameCtx && FuncNameCtx->CF;
  }

  bool InOverloadResolGuide() const {
    return FuncNameCtx && FuncNameCtx->Sig;
  }

  //
  // IsAllowedNoEnumeratorArguments:
  //
  // データ型が定義されている列挙子を参照するとき，データを
  // 初期化する引数が与えられていない構文を許可するかどうか
  //
  // used for:
  //   match-statement
  //
  bool IsAllowedNoEnumeratorArguments = false;
};

struct SemaIdentifierEvalResult {

  /*
    enum IDEvalResult {
      IE_None,

      IE_Identifier,

      IE_Error,
    };

    IDEvalResult Result = IE_None;

    */

  IdentifierInfo II;
  TypeInfo Type;

  ASTPtr<AST::Identifier> ast;
};

/*
 * TemplateInstantiationContext:
 *
 * テンプレート関数・クラスのインスタンス化を行う．
 * また，それに関する情報や文脈を管理する．
 *
 */
class TemplateInstantiationContext {
  friend class Sema;

public:
  //
  // InstantiationResultTypes
  //
  // 与えられたテンプレート引数と，定義側のパラメータとの比較．
  //
  enum InstantiationResultTypes {

    //
    // このクラスのインスタンスを作成した直後．
    // まだインスタンス化をしていない状態．
    TI_NotTryied,

    //
    // 正しくインスタンス化できた．
    TI_Succeed,

    //
    // 与えられたテンプレート引数が多すぎる
    TI_TooManyParameters,

    //
    // 不足しているテンプレート引数の型を他の文脈から推論できなかった．
    TI_CannotDeductType,

    TI_Arg_ParamError,

    TI_Arg_TypeMismatch,

    TI_Arg_TooFew,

    TI_Arg_TooMany,
  };

  //
  // FunctionInstantiationResultTypes
  //
  // テンプレート関数のインスタンス化を行ったあとの結果．
  //
  enum FunctionInstantiationResultTypes {
    TI_FR_NotTryied,

    //
    // 成功
    TI_FR_Succeed,

    //
    // 引数の型が一致しない
    TI_FR_Arg_TypeMismatch,

    //
    // 引数が少ない
    TI_FR_Arg_TooFew,

    //
    // 引数が多すぎる
    TI_FR_Arg_TooMany,
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
    InstantiationResultTypes Type = TI_NotTryied;

    FunctionInstantiationResultTypes FuncResult = TI_FR_NotTryied;

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

  ASTPtr<AST::Function> TryInstantiate_Of_Function(SemaFunctionNameContext const* Ctx);

  ASTPtr<AST::Class> TryInstantiate_Of_Class();

  TemplateInstantiationContext(Sema& S, IdentifierInfo* ParamsII,
                               ASTPtr<AST::Templatable> Item);
};

class Sema {

  friend struct BlockScope;
  friend struct FunctionScope;

  friend struct SemaContext;
  friend struct SemaFunctionNameContext;
  friend struct SemaMemberReferenceContext;

  friend class TemplateInstantiationContext;

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
    return this->get_identifier_info(AST::GetID(ast));
  }

  IdentifierInfo GetIdentifierInfo(ASTPtr<AST::Identifier> Id, SemaContext& Ctx);

  SemaIdentifierEvalResult EvalID(ASTPtr<AST::Identifier> id, SemaContext& Ctx);

  Vec<std::shared_ptr<SemaIdentifierEvalResult>> EvaluatedIDResultRecord;

  // -----------------
  // -----------------
  size_t GetMatchedFunctions(ASTVec<AST::Function>& Matched,
                             ASTVec<AST::Function> const& Candidates,
                             IdentifierInfo* ParamsII, SemaContext const* Ctx,
                             bool ThrowError = false);

public:
  Sema(ASTPtr<AST::Block> prg);
  ~Sema();

  void check_full();

  void check(ASTPointer ast, SemaContext Ctx = {});

  TypeInfo eval_type(ASTPointer ast, SemaContext Ctx = {});

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

  void ClearScopeHistory();

  void SaveScopeLocation();
  void RestoreScopeLocation();

  void BackToDepth(int depth);
  bool BackTo(ScopeContext* ctx);

  int GetScopesOfDepth(vector<ScopeContext*>& out, ScopeContext* scope, int depth);

  TypeInfo ExpectType(TypeInfo const& type, ASTPointer ast);

  TypeInfo make_functor_type(ASTPtr<AST::Function> ast);
  TypeInfo make_functor_type(builtins::Function const* builtin);

  std::map<ASTPtr<AST::Class>, bool> _class_analysing_flag_map;

  static bool IsDerivedFrom(ASTPtr<AST::Class> _class, ASTPtr<AST::Class> _base);
};

} // namespace fire::semantics_checker