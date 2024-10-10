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

class Sema {

  friend struct BlockScope;
  friend struct FunctionScope;

  ArgumentCheckResult check_function_call_parameters(ASTVector args, bool isVariableArg,
                                                     TypeVec const& formal,
                                                     TypeVec const& actual,
                                                     bool ignore_mismatch);

  LocalVar* _find_variable(string_view const& name);
  ASTVec<Function> _find_func(string_view const& name);
  ASTPtr<Enum> _find_enum(string_view const& name);
  ASTPtr<Class> _find_class(string_view const& name);
  ASTPtr<Block> _find_namespace(string_view const& name);

  ASTPointer context_reverse_search(std::function<bool(ASTPointer)> func);

  NameFindResult find_name(string_view const& name, bool const only_cur_scope = false);

  IdentifierInfo get_identifier_info(ASTPtr<AST::Identifier> ast,
                                     bool only_cur_scope = false);

  IdentifierInfo get_identifier_info(ASTPtr<AST::ScopeResol> ast);

  IdentifierInfo get_identifier_info(ASTPointer ast) {
    if (ast->kind == ASTKind::ScopeResol)
      return this->get_identifier_info(ASTCast<AST::ScopeResol>(ast));

    return this->get_identifier_info(Sema::GetID(ast));
  }

  struct SemaContext {

    FunctionScope* original_template_func = nullptr;

    bool create_new_scope = false;

    struct FunctionNameContext {
      ASTPtr<AST::CallFunc> CF;
      ASTPtr<AST::Signature> Sig;

      TypeVec const* ArgTypes;
      TypeInfo const* ExpectedResultType;

      bool IsValid() {
        return CF || Sig;
      }

      size_t GetArgumentsCount() {
        return ArgTypes ? ArgTypes->size() : 0;
      }
    };

    FunctionNameContext FuncName;

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

      TI_Arg_TooFew,

      TI_Arg_TooMany,
    };

    struct ParameterInfo {
      string Name;
      TypeInfo Type;

      Token* Tok = nullptr;

      bool IsAssignmented = false;

      bool IsDeducted = false;

      //
      // テンプレート引数の指定部分 "@< ... >" で渡されたものは IsAssignmented = true
      // となる
      //
      // 指定されずに、引数から型推論した場合は、IsDeducted = true になる。
      //
      // IsAssignmented = true
      // のときは、引数の型と完全一致ではなく、構文に当てはめて正しい意味になるかどうかを確認する。
      //

      ASTPointer Given = nullptr;

      Vec<ParameterInfo> Params;

      ParameterInfo(string const& Name, TypeInfo const& T = {})
          : Name(Name),
            Type(T) {
      }

      ParameterInfo(Token* token)
          : Name(string(token->str)),
            Tok(token) {
      }

      ParameterInfo(AST::Templatable::ParameterName const& P)
          : ParameterInfo(P.token) {
        for (auto&& pp : P.params) {
          this->Params.emplace_back(pp);
        }
      }
    };

    struct PseudoTypeInfo {
      string name;

      Token* token;

      Vec<PseudoTypeInfo> params;

      PseudoTypeInfo(string const& name, Token* tok = nullptr,
                     Vec<PseudoTypeInfo> pp = {})
          : name(name),
            token(tok),
            params(std::move(pp)) {
      }

      PseudoTypeInfo(ASTPtr<AST::TypeName> ast)
          : PseudoTypeInfo(string(ast->GetName()), &ast->token) {

        for (auto&& p : ast->type_params)
          this->params.emplace_back(p);
      }
    };

    enum PseudoTypeReplacedResult {
      REPL_Didnt,

      REPL_Succeed,

      REPL_InvalidName,

      REPL_NotTemplate, // but given params

      REPL_TooFewParam,

      REPL_TooManyParam,
    };

    struct TIReplacementResult {
      PseudoTypeReplacedResult Result;

      PseudoTypeInfo* Type;

      TypeInfo Replaced;

      TIReplacementResult(PseudoTypeReplacedResult R, PseudoTypeInfo* T)
          : Result(R),
            Type(T) {
      }
    };

    struct InstantiationResult {
      ResultTypes Type = TI_NotTryied;

      ParameterInfo* ErrParam = nullptr;

      ASTPointer ErrArgument = nullptr;

      size_t ErrArgIndex = 0;
    };

    Sema& S;

    IdentifierInfo* ParamsII;

    ASTPtr<AST::Templatable> Item; // Templated (original)

    InstantiationResult TryiedResult;

    Vec<ParameterInfo> Params;

    TemplateInstantiationContext* _Previous = nullptr;

    ParameterInfo* GetParam(string const& name) {
      for (auto&& P : this->Params)
        if (P.Name == name)
          return &P;

      return nullptr;
    }

    void CheckParameterMatchingToTypeDecl(ParameterInfo const& P,
                                          ASTPtr<AST::TypeName> T) {

      if (P.Name != T->GetName())
        return;

      if (P.Params.empty()) {
        if (T->type_params.size() >= 1) {
          throw Error(T->token, "'" + P.Name + "' is not a templated");
        }
      }
      else if (P.Params.size() < T->type_params.size()) {
        throw Error(T->token, "too many template arguments");
      }
      else if (P.Params.size() > T->type_params.size()) {
        throw Error(T->token, "too few template arguments");
      }

      for (size_t i = 0; i < P.Params.size(); i++) {
        CheckParameterMatchingToTypeDecl(P.Params[i], T->type_params[i]);
      }
    }

    TIReplacementResult ReplacePseudoTI(PseudoTypeInfo& P) {

      TIReplacementResult R{REPL_Didnt, &P};

      alertexpr(P.name);

      R.Replaced = TypeInfo::from_name(P.name);

      int expected_param_count = 0;

      switch (R.Replaced.kind) {
      case TypeKind::Unknown: {
        if (auto PI = this->GetParam(P.name); PI && PI->IsAssignmented) {
          alert;
          R.Replaced.kind = PI->Type.kind;
        }
        else if (auto _E = S._find_enum(P.name); _E) {
          R.Replaced = TypeInfo::from_enum(_E);
        }
        else if (auto _C = S._find_class(P.name); _C) {
          R.Replaced = TypeInfo::from_class(_C);
        }
      }

      case TypeKind::Vector:
      case TypeKind::Tuple:
      case TypeKind::Dict:
        alert;
        expected_param_count = R.Replaced.needed_param_count();
        break;

      case TypeKind::Function:
        todo_impl;

      default:
        alert;
      }

      alertexpr(expected_param_count);

      if (expected_param_count != -1) {
        if (expected_param_count < (int)P.params.size()) {
          R.Result = REPL_TooFewParam;
          return R;
        }
        else if ((int)P.params.size() < expected_param_count) {
          R.Result = REPL_TooManyParam;
          return R;
        }
      }

      for (auto&& Param : P.params) {

        auto pr = ReplacePseudoTI(Param);

        if (pr.Result == REPL_Succeed) {
          R.Replaced.params.emplace_back(pr.Replaced);
        }

        else {
          return pr;
        }
      }

      R.Result = REPL_Succeed;

      return R;
    }

    ASTPtr<AST::Function>
    TryInstantiate_Of_Function(SemaContext::FunctionNameContext* Ctx) {

      alert;

      if (Item->kind != ASTKind::Function)
        return nullptr;

      auto Func = ASTCast<AST::Function>(Item);

      alert;

      // 可変長じゃないのにテンプレート引数が多い
      if (!Func->IsVariableParameters &&
          Func->ParameterCount() < ParamsII->id_params.size()) {
        TryiedResult.Type = TI_TooManyParameters;
        return nullptr;
      }

      // 引数が与えられている文脈
      if (Ctx && Ctx->ArgTypes) {
        alert;

        if (Ctx->GetArgumentsCount() < Func->arguments.size())
          return nullptr;

        if (!Func->is_var_arg && Func->arguments.size() < Ctx->GetArgumentsCount())
          return nullptr;

        for (size_t i = 0; auto&& ArgType : *Ctx->ArgTypes) {
          ASTPtr<AST::Argument>& Formal = Func->arguments[i];

          auto P = this->GetParam(string(Formal->type->GetName()));

          if (!P) {
            alert;

            continue;
          }

          CheckParameterMatchingToTypeDecl(*P, Formal->type);

          if (P->IsDeducted && !P->Type.equals(ArgType)) {
            alert;

            TryiedResult.Type = TI_Arg_TypeMismatch;

            TryiedResult.ErrParam = P;

            TryiedResult.ErrArgument =
                Ctx->CF ? Ctx->CF->args[i] : Ctx->Sig->arg_type_list[i];

            TryiedResult.ErrArgIndex = i;

            return nullptr;
          }

          if (P->IsAssignmented) {
            alert;
            PseudoTypeInfo psuedo_ti{Formal->type};

            if (P->Type.params.size() >= 1 && Formal->type->type_params.size() >= 1) {
              todo_impl;
            }

            auto result = ReplacePseudoTI(psuedo_ti);

            if (result.Result == REPL_Succeed) {
              alert;
            }
            else {
              todo_impl;
            }
          }

          else {

            alertexpr(ArgType.GetName());
          }
        }
      }

      alert;
      return nullptr;
    }

    ASTPtr<AST::Class> TryInstantiate_Of_Class() {

      todo_impl;
    }

    TemplateInstantiationContext(Sema& S, IdentifierInfo* ParamsII,
                                 ASTPtr<AST::Templatable> Item)
        : S(S),
          ParamsII(ParamsII),
          Item(Item) {
      for (auto&& P : Item->template_param_names) {
        this->Params.emplace_back(P);
      }

      alert;
      for (size_t i = 0; auto&& ParamType : ParamsII->id_params) {
        this->Params[i].Type = ParamType;
        this->Params[i].Given = ParamsII->template_args[i].ast;
        this->Params[i].IsAssignmented = true;

        i++;
      }
    }
  };

  friend struct TemplateInstantiationContext;

  // -----------------
  // -----------------
  size_t GetMatchedFunctions(ASTVec<AST::Function>& Matched,
                             ASTVec<AST::Function> const& Candidates,
                             IdentifierInfo* ParamsII, SemaContext* Ctx) {

    for (auto&& C : Candidates) {

      // テンプレート関数の場合、いったんインスタンス化してみる
      // 成功したら候補に追加する
      if (C->is_templated) {
        TemplateInstantiationContext TI{*this, ParamsII, C};

        if (auto f = TI.TryInstantiate_Of_Function(Ctx ? &Ctx->FuncName : nullptr); f) {
          Matched.emplace_back(f);
          continue;
        }
      }
    }

    return Matched.size();
  }

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
  struct TemplateTypeApplier {

    enum class Result {
      //
      // just after created instance
      NotApplied,

      //
      // applied parameter types, but not checked.
      NotChecked,

      TooManyParams,

      CannotDeductType,

      ArgumentError,
    };

    struct Parameter {
      string_view name;
      TypeInfo type;

      Parameter(string_view name, TypeInfo type)
          : name(name),
            type(type) {
        alertexpr(name);
      }
    };

    Sema* S;

    ASTPtr<AST::Templatable> ast;

    ASTPointer Instantiated = nullptr;

    vector<Parameter> parameter_list;

    Result result = Result::NotApplied;
    size_t err_param_index = 0;

    ArgumentCheckResult arg_result = ArgumentCheckResult::None;

    // index of error item
    size_t index = 0;

    Parameter& add_parameter(string_view name, TypeInfo type);
    Parameter* find_parameter(string_view name);

    //
    // ast = error location
    Error make_error(ASTPointer ast);

    TemplateTypeApplier();
    TemplateTypeApplier(ASTPtr<AST::Templatable> ast,
                        vector<Parameter> parameter_list = {});
    ~TemplateTypeApplier();
  };

  friend struct TemplateTypeApplier;

  std::vector<TemplateTypeApplier> _applied_templates;

  std::list<TemplateTypeApplier*> _applied_ptr_stack;

  Vec<ASTPtr<AST::Templatable>> InstantiatedTemplates; // to check

  TypeInfo* find_template_parameter_name(string_view const& name);

  //
  // args = template arguments (parameter)
  //
  TemplateTypeApplier apply_template_params(ASTPtr<AST::Templatable> ast,
                                            TypeVec const& args);

  bool try_apply_template_function(SemaContext*, TemplateTypeApplier&,
                                   ASTPtr<AST::Function>, TypeVec const&,
                                   TypeVec const&) {

    return true;
  }

  TemplateTypeApplier* _find_applied(TemplateTypeApplier const& t);

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