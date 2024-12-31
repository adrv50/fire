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
  BuiltinMemberVar,

  Func,
  MemberFunc,

  BuiltinFunc,
  BuiltinMethod,

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
  Vec<builtins::Function const*> builtin_funcs;

  builtins::MemberVariable const* builtin_attr = nullptr;

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
  ASTPtr<AST::Identifier> ast = nullptr;

  TypeVec id_params = {};

  Vec<IdentifierInfo> template_args = {};

  NameFindResult result = {};

  string to_string() const;
};

struct SemaFunctionNameContext {
  ASTPtr<AST::CallFunc> CF = nullptr;
  ASTPtr<AST::Signature> Sig = nullptr;

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

  ASTPtr<AST::Class> Now_Analysing = nullptr;

  ASTPtr<AST::Class> InheritBaseClass = nullptr;

  bool PassMemberAnalyze = false;
};

//
// = SemaExprContext =
//
// 特定の文脈において式の型を評価するときに使用されます．
//
struct SemaExprContext {

  enum Kind {
    EX_None,
    EX_MemberRef,  // ASTKind::MemberAccess
    EX_Assignment, // ASTKind::Assign
  };

  Kind kind = EX_None;

  ASTPtr<AST::Expr> E;

  ASTPointer Left;
  ASTPointer Right;

  //
  // -- AssignRightTypeToLVar
  //
  // 代入式において左辺が変数で，かつそれの型がまだ不明なとき，
  // 右辺の型を評価し，それを割り当てます．
  //
  // If in an assignment expression has variable name in left side,
  // set the type of right side to that if type not deducted yet.
  //
  bool AssignRightTypeToLVar = false; //        |
                                      //        |
  LocalVar* TargetLVarPtr = nullptr;  //  <-----

  TypeInfo LeftType = {};
  TypeInfo RightType = {};

  ASTPtr<AST::Identifier> LeftID = nullptr;

  //
  // 特定の文脈内において，TypeKind::Instance として評価される式があり，
  // そのクラスへのポインタが必要なとき，これを経由します．
  bool IsClassInfoNeeded = false;
  ASTPointer InstancableExprAST = nullptr;           // expr evaluated to instance object.
  ASTPtr<AST::Class> ExprInstanceClassPtr = nullptr; // the class type of expr pointed to.
};

struct SemaScopeResolContext {

  // todo

  IdentifierInfo* LeftII = nullptr;

  ASTPtr<AST::Class> GetClassAST() {
    return LeftII ? LeftII->result.ast_class : nullptr;
  }
};

struct SemaTypeExpectionContext {

  TypeInfo const* Expected = nullptr;
};

struct SemaContext {

  //
  // 関数呼び出し式において，呼び出し先の関数を一つに決める必要がある場合に使用
  SemaFunctionNameContext* FuncNameCtx = nullptr;

  //
  // クラス名として参照される式である場合に使用
  // また，クラス内を解析している場合は，そのクラスへのポインタを格納．
  SemaClassNameContext* ClassCtx = nullptr;

  SemaExprContext* ExprCtx = nullptr;

  SemaScopeResolContext* ScopeResolCtx = nullptr;

  SemaTypeExpectionContext* TypeExpection = nullptr;

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

  bool InCallFunc() const {
    return FuncNameCtx && FuncNameCtx->CF;
  }

  bool InOverloadResolGuide() const {
    return FuncNameCtx && FuncNameCtx->Sig;
  }
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

namespace eval {
class Evaluator;
}

class Sema {

  friend class eval::Evaluator;

  friend struct BlockScope;
  friend struct FunctionScope;

  friend struct SemaContext;
  friend struct SemaFunctionNameContext;
  friend struct SemaExprContext;

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

  SemaIdentifierEvalResult& EvalID(ASTPtr<AST::Identifier> id, SemaContext& Ctx);

  Vec<std::shared_ptr<SemaIdentifierEvalResult>> EvaluatedIDResultRecord;

  // -----------------
  // -----------------
  size_t GetMatchedFunctions(ASTVec<AST::Function>& Matched,
                             ASTVec<AST::Function> const& Candidates,
                             IdentifierInfo* ParamsII, SemaContext const* Ctx,
                             bool ThrowError = false);

  enum fn_sig_type_e {
    FSIG_TYPE_TEMPORARY, // temporary constructed data

    FSIG_TYPE_DEFINE, // definition of function

    FSIG_TYPE_CALL, // call-expr
  };

  struct func_signature_info {
    ASTPointer ast = nullptr;

    fn_sig_type_e this_type = FSIG_TYPE_TEMPORARY;

    bool is_variable_args = false;

    Vec<TypeInfo> arg_types;

    TypeInfo result_type;
  };

  Vec<TypeInfo> _create_type_vec_from__func_def(ASTPtr<AST::Function> func) {
    Vec<TypeInfo> v;

    for (auto&& a : func->arguments)
      v.emplace_back(this->eval_type(a->type));

    return v;
  }

  Vec<TypeInfo> _create_type_vec_from__call_expr(ASTPtr<AST::CallFunc> c) {
    Vec<TypeInfo> v;

    for (auto&& a : c->args)
      v.emplace_back(this->eval_type(a));

    return v;
  }

  func_signature_info create_signature_from_definition(ASTPtr<AST::Function> func) {
    return func_signature_info{.ast = func,
                               .this_type = FSIG_TYPE_DEFINE,
                               .is_variable_args = func->is_var_arg,
                               .arg_types = _create_type_vec_from__func_def(func),
                               .result_type = this->eval_type(func->return_type)};
  }

  func_signature_info create_signature_from_call_expr(ASTPtr<AST::CallFunc> c) {
    func_signature_info sig;

    sig.ast = c;

    sig.this_type = FSIG_TYPE_CALL;

    sig.is_variable_args =
        (c->callee_builtin
             ? c->callee_builtin->is_variable_args
             : (c->callee ? (c->callee->Is(ASTKind::Function)
                                 ? c->callee->As<AST::Function>()->is_var_arg
                                 : (false /*<-- case of c->kind is not Function*/))
                          : false));

    sig.arg_types = _create_type_vec_from__func_def(c->callee_ast);

    sig.result_type =
        (c->callee
             ? (c->callee->Is(ASTKind::Function)
                    ? this->eval_type(c->callee->As<AST::Function>()->return_type)
                    : (TypeKind::None /*c->callee is valid but c->kind is not function*/))
             : (c->callee_builtin
                    ? c->callee_builtin->result_type
                    : (TypeKind::None /*c->callee and c->callee_builtin is null. */)));

    return sig;
  }

  enum fn_sig_cmp_result_enum {
    //
    // Perfect match (No problems.)
    FSIG_CMP_EQ,

    //
    // Mismatched type of argument at index
    FSIG_CMP_ARG_TYPE_MISMATCH,

    //
    // Too few args
    FSIG_CMP_ARG_TOO_FEW,

    //
    // Too many args
    FSIG_CMP_ARG_TOO_MANY,
  };

  struct func_sig_compare_result {

    func_signature_info A;
    func_signature_info B;

    fn_sig_cmp_result_enum result;

    i64 index = 0; // for argument error
  };

public:
  Sema(ASTPtr<AST::Block> prg);
  ~Sema();

  void check_full();

  void check(ASTPointer ast, SemaContext Ctx = {});

  TypeInfo eval_type(ASTPointer ast, SemaContext Ctx = {});

  TypeInfo eval_type_name(ASTPtr<AST::TypeName> ast);

  TypeInfo EvalExpr(ASTPtr<AST::Expr> ast);

  static bool IsWritable(ASTPointer ast);

  static Sema* GetInstance();

private:
  struct TemplateInstantiatedRecord {
    ASTPtr<AST::Templatable> Original;
    ASTPtr<AST::Templatable> Instantiated;

    std::list<ScopeContext*> Scope;
  };

  Vec<TemplateInstantiatedRecord> InstantiatedRecords;

  TemplateInstantiatedRecord* find_instantiated(ASTPtr<AST::Templatable> original) {
    for (auto&& ti : this->InstantiatedRecords)
      if (ti.Original == original)
        return &ti;

    return nullptr;
  }

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