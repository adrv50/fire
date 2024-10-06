#pragma once

#include <any>
#include <algorithm>
#include <concepts>
#include <string>
#include <fstream>
#include <functional>
#include <cassert>

#include "alert.h"
#include "Object.h"
#include "Token.h"

namespace fire {

enum class ASTKind {
  Value,

  Identifier, // => AST::Identifier
  ScopeResol, // => AST::ScopeResol

  //
  // /--------------
  //  replaced to this kind from Identifier or ScopeResol in
  //  Semantics-Checker. Don't use this.
  Variable,
  FuncName,
  BuiltinFuncName,
  Enumerator,
  EnumName,
  ClassName,
  // ------------------/

  LambdaFunc, // => AST::Function

  OverloadResolutionGuide, // "of"

  Array,

  IndexRef,

  MemberAccess,

  // /-----------------
  MemberVariable, // => use ast->GetID() (to index in instance)
  MemberFunction,
  BuiltinMemberVariable,
  BuiltinMemberFunction,
  //   ------------------/

  CallFunc,

  CallFunc_Ctor,

  //
  // in call-func expr.
  SpecifyArgumentName, // => AST::Expr

  Not, // => todo impl.

  Mul,
  Div,
  Mod,

  Add,
  Sub,

  LShift,
  RShift,

  //
  // Compare
  //
  Bigger,        // a > b
  BiggerOrEqual, // a >= b
  Equal,
  // NotEqual  => replace: (a != b) -> (!(a == b))

  BitAND,
  BitXOR,
  BitOR,

  LogAND,
  LogOR,

  Assign,

  Block,
  Vardef,

  If,

  Switch,
  While,

  Break,
  Continue,

  Return,

  Throw,
  TryCatch,

  Argument,
  Function,

  Enum,
  Class,

  Namespace,

  TypeName,

  Signature,
};

namespace AST {

template <typename T, typename U>
ASTVec<T> CloneASTVec(ASTVec<U> const& vec) {
  ASTVec<T> v;

  for (auto&& elem : vec)
    v.emplace_back(ASTCast<T>(elem->Clone()));

  return v;
}

struct Base {
  ASTKind kind;
  Token token;
  Token endtok;

  bool is_named = false;
  bool is_expr = false;

  ASTKind _constructed_as;

  bool is_ident_or_scoperesol() const {
    return this->_constructed_as == ASTKind::Identifier ||
           this->_constructed_as == ASTKind::ScopeResol;
  }

  template <class T>
  T* As() {
    return static_cast<T*>(this);
  }

  template <class T>
  T const* As() const {
    return static_cast<T const*>(this);
  }

  Expr const* as_expr() const;
  Statement const* as_stmt() const;

  Expr* as_expr();
  Statement* as_stmt();

  Value const* as_value() const;

  virtual Identifier* GetID();

  virtual ASTPointer Clone() const = 0;

  [[maybe_unused]] i64 GetChilds(ASTVector& out) const;

  virtual ~Base() = default;

protected:
  Base(ASTKind kind, Token token);
  Base(ASTKind kind, Token token, Token endtok);
};

struct Value : Base {
  ObjPointer value;

  static ASTPtr<Value> New(Token tok, ObjPointer val);

  ASTPointer Clone() const override;

  Value(Token tok, ObjPointer value);
};

struct Named : Base {
  Token name;

  string_view const& GetName() const;

protected:
  Named(ASTKind kind, Token tok, Token name);

  Named(ASTKind kind, Token tok);
};

struct Identifier : Named {
  Token paramtok; // "<"

  ASTVector id_params; // T = Identifier or ScopeResol

  // flags for Sema
  bool sema_must_completed = true;
  bool sema_allow_ambiguous = false;
  bool sema_use_keeped = false;

  //
  // for Kind::FuncName or BuiltinFuncName
  ASTVec<Function> candidates;
  vector<builtins::Function const*> candidates_builtin;

  TypeInfo ft_ret;
  vector<TypeInfo> ft_args;

  vector<TypeInfo> template_args;

  //
  // for BuiltinMemberVariable
  builtins::MemberVariable const* blt_member_var = nullptr;

  //
  // for Kind::Variable
  int distance = 0;
  int index = 0; // (=> or member variable, enumerator)
  int index_add = 0;

  ASTPtr<Class> ast_class = nullptr;
  ASTPtr<Enum> ast_enum = nullptr;

  TypeInfo self_type; // if member

  static ASTPtr<Identifier> New(Token tok);

  ASTPointer Clone() const override;

  Identifier* GetID() override;

  Identifier(Token tok);
};

struct ScopeResol : Named {
  ASTPtr<Identifier> first;
  ASTVec<Identifier> idlist;

  static ASTPtr<ScopeResol> New(ASTPtr<Identifier> first);

  ASTPointer Clone() const override;

  Identifier* GetID() override;

  ASTPtr<Identifier> GetLastID() const;

  ScopeResol(ASTPtr<Identifier> first);
};

struct Array : Base {
  ASTVector elements;

  TypeInfo elem_type; // set in Sema

  static ASTPtr<Array> New(Token tok);

  ASTPointer Clone() const override;

  Array(Token tok);
};

struct CallFunc : Base {
  ASTPointer callee; // left side, evaluated to be callable object.
  ASTVector args;

  ASTPtr<Function> callee_ast = nullptr;
  builtins::Function const* callee_builtin = nullptr;

  bool call_functor = false;

  static ASTPtr<CallFunc> New(ASTPointer callee, ASTVector args = {});

  ASTPointer Clone() const override;

  ASTPtr<Class> get_class_ptr() const;

  CallFunc(ASTPointer callee, ASTVector args = {});
};

struct Expr : Base {
  Token op;
  ASTPointer lhs;
  ASTPointer rhs;

  static ASTPtr<Expr> New(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs);

  ASTPointer Clone() const override;

  Identifier* GetID() override;

  Expr(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs);
};

struct Block : Base {
  ASTVector list;
  int stack_size = 0; // count of variable definition

  static ASTPtr<Block> New(Token tok, ASTVector list = {});

  ASTPointer Clone() const override;

  Block(Token tok, ASTVector list = {});
};

struct TypeName : Named {
  ASTPtr<Signature> sig;

  ASTVec<TypeName> type_params;
  bool is_const;

  TypeInfo type;
  ASTPtr<Class> ast_class = nullptr;

  static ASTPtr<TypeName> New(Token nametok);

  ASTPointer Clone() const override;

  TypeName(Token name);
};

struct Signature : Base {
  ASTVec<TypeName> arg_type_list;
  ASTPtr<TypeName> result_type;

  static ASTPtr<Signature> New(Token tok, ASTVec<TypeName> arg_type_list,
                               ASTPtr<TypeName> result_type);

  ASTPointer Clone() const override;

  Signature(Token tok, ASTVec<TypeName> arg_type_list, ASTPtr<TypeName> result_type);
};

struct VarDef : Named {
  ASTPtr<TypeName> type;
  ASTPointer init;

  int index = 0;
  int index_add = 0;

  static ASTPtr<VarDef> New(Token tok, Token name, ASTPtr<TypeName> type,
                            ASTPointer init);

  static ASTPtr<VarDef> New(Token tok, Token name) {
    return New(tok, name, nullptr, nullptr);
  }

  ASTPointer Clone() const override;

  VarDef(Token tok, Token name, ASTPtr<TypeName> type = nullptr,
         ASTPointer init = nullptr);
};

struct Statement : Base {
  struct If {
    ASTPointer cond, if_true, if_false;
  };

  struct Switch {
    struct Case {
      ASTPointer expr, block;
    };

    ASTPointer cond;
    std::vector<Case> cases;
  };

  struct While {
    ASTPointer cond;
    ASTPtr<Block> block;
  };

  struct TryCatch {
    struct Catcher {
      Token varname; // name of variable to catch exception instance
      ASTPtr<TypeName> type;

      ASTPtr<Block> catched;

      TypeInfo _type;
    };

    ASTPtr<Block> tryblock;
    std::vector<Catcher> catchers;
  };

  template <typename T>
  T get_data() const {
    return std::any_cast<T>(this->_astdata);
  }

  template <typename T>
  void set_data(T&& data) {
    this->_astdata = std::forward<T>(data);
  }

  ASTPtr<AST::Expr> get_expr() const {
    return ASTCast<AST::Expr>(this->get_data<ASTPointer>());
  }

  static ASTPtr<Statement> NewIf(Token tok, ASTPointer cond, ASTPointer if_true,
                                 ASTPointer if_false = nullptr);

  static ASTPtr<Statement> NewSwitch(Token tok, ASTPointer cond,
                                     std::vector<Switch::Case> cases = {});

  static ASTPtr<Statement> NewWhile(Token tok, ASTPointer cond, ASTPtr<Block> block);

  static ASTPtr<Statement> NewTryCatch(Token tok, ASTPtr<Block> tryblock,
                                       vector<TryCatch::Catcher> catchers);

  static ASTPtr<Statement> New(ASTKind kind, Token tok,
                               std::any data = (ASTPointer) nullptr);

  ASTPointer Clone() const override;

  Statement(ASTKind kind, Token tok, std::any data);

private:
  std::any _astdata;
};

struct Templatable : Named {
  Token tok_template;

  bool is_templated = false;

  TokenVector template_param_names;

protected:
  using Named::Named;

  void _Copy(Templatable const* _t);
};

struct Argument : Named {
  ASTPtr<TypeName> type;

  static ASTPtr<Argument> New(Token nametok, ASTPtr<TypeName> type);

  ASTPointer Clone() const;

  Argument(Token nametok, ASTPtr<TypeName> type);
};

struct Function : Templatable {
  ASTVec<Argument> arguments;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  bool is_var_arg;

  ASTPtr<Class> member_of = nullptr;

  static ASTPtr<Function> New(Token tok, Token name);

  static ASTPtr<Function> New(Token tok, Token name, ASTVec<Argument> args,
                              bool is_var_arg, ASTPtr<TypeName> rettype,
                              ASTPtr<Block> block);

  ASTPtr<Argument>& add_arg(Token const& tok, ASTPtr<TypeName> type = nullptr);

  ASTPtr<Argument> find_arg(std::string const& name);

  ASTPointer Clone() const override;

  Function(Token tok, Token name);
  Function(Token tok, Token name, ASTVec<Argument> args, bool is_var_arg,
           ASTPtr<TypeName> rettype, ASTPtr<Block> block);
};

struct Enum : Templatable {
  struct Enumerator {
    Token name;
    ASTPtr<TypeName> type = nullptr;
  };

  vector<Enumerator> enumerators;

  Enumerator& append(Token name, ASTPtr<TypeName> type);
  Enumerator& append(Enumerator const& e);

  static ASTPtr<Enum> New(Token tok, Token name);

  ASTPointer Clone() const override;

  Enum(Token tok, Token name);
};

struct Class : Templatable {
  ASTVec<VarDef> member_variables;
  ASTVec<Function> member_functions;

  static ASTPtr<Class> New(Token tok, Token name);

  static ASTPtr<Class> New(Token tok, Token name, ASTVec<VarDef> member_variables,
                           ASTVec<Function> member_functions);

  ASTPtr<VarDef>& append_var(ASTPtr<VarDef> ast);

  ASTPtr<Function>& append_func(ASTPtr<Function> ast);

  ASTPointer Clone() const override;

  Class(Token tok, Token name, ASTVec<VarDef> member_variables = {},
        ASTVec<Function> member_functions = {});
};

enum ASTWalkerLocation {
  AW_Begin,
  AW_End,
};

void walk_ast(ASTPointer ast, std::function<void(ASTWalkerLocation, ASTPointer)> fn);

string ToString(ASTPointer ast);

} // namespace AST

} // namespace fire
