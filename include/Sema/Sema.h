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

  ArgumentCheckResult check_function_call_parameters(ASTVector args, bool isVariableArg,
                                                     TypeVec const& formal,
                                                     TypeVec const& actual,
                                                     bool ignore_mismatch);

  ScopeContext::LocalVar* _find_variable(string_view const& name);
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

    FunctionScope* swap_func_scope;
  };

public:
  Sema(ASTPtr<AST::Block> prg);
  ~Sema();

  void check_full();

  void check(ASTPointer ast, SemaContext Ctx = {0});

  TypeInfo eval_type(ASTPointer ast, SemaContext Ctx = {0});

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

  bool try_apply_template_function(TemplateTypeApplier& out, ASTPtr<AST::Function> ast,
                                   TypeVec const& args, TypeVec const& func_args);

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