#pragma once

#include <any>
#include <algorithm>
#include <concepts>
#include <string>
#include <fstream>
#include <functional>

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
  Enumerator,
  // ------------------/

  Array,

  IndexRef,
  MemberAccess,
  CallFunc,

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
  For,

  Break,
  Continue,
  Return,

  Throw,

  TryCatch,

  Function,

  Enum,
  Class,

  TypeName,

  Program,
};

namespace AST {

struct Base {
  ASTKind kind;
  Token token;
  Token endtok;

  bool is_named = false;
  bool is_expr = false;

  ASTKind _constructed_as;

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

  Value const* as_value() const {
    return (Value const*)this;
  }

  [[maybe_unused]]
  i64 GetChilds(ASTVector& out) const;

  virtual std::string_view GetSourceView() const {
    return token.sourceloc.GetLine();
  }

  virtual ~Base() = default;

protected:
  Base(ASTKind kind, Token token)
      : Base(kind, token, token) {
  }

  Base(ASTKind kind, Token token, Token endtok)
      : kind(kind),
        token(token),
        endtok(endtok),
        _constructed_as(kind) {
  }
};

struct Value : Base {
  ObjPointer value;

  static ASTPtr<Value> New(Token tok, ObjPointer val);

  Value(Token tok, ObjPointer value)
      : Base(ASTKind::Value, tok),
        value(value) {
  }
};

struct Named : Base {
  Token name;

  std::string GetName() const {
    return this->name.str;
  }

protected:
  Named(ASTKind kind, Token tok, Token name)
      : Base(kind, tok),
        name(name) {
    this->is_named = true;
  }

  Named(ASTKind kind, Token tok)
      : Named(kind, tok, tok) {
  }
};

struct Variable : Named {
  int index = 0;
  int backstep = 0; // N 個前のスコープに戻る

  static ASTPtr<Variable> New(Token tok);

  Variable(Token tok)
      : Named(ASTKind::Variable, tok, tok) {
  }
};

struct Identifier : Named {
  Token paramtok; // "<"
  ASTVec<TypeName> id_params;

  // for FuncName
  ASTVec<Function> func_candidates;

  static ASTPtr<Identifier> New(Token tok);

  Identifier(Token tok)
      : Named(ASTKind::Identifier, tok, tok) {
  }
};

struct ScopeResol : Named {
  ASTPtr<Identifier> first;
  ASTVec<Identifier> idlist;

  static ASTPtr<ScopeResol> New(ASTPtr<Identifier> first);

  ScopeResol(ASTPtr<Identifier> first)
      : Named(ASTKind::ScopeResol, first->token),
        first(first) {
  }
};

struct Array : Base {
  ASTVector elements;

  static ASTPtr<Array> New(Token tok);

  Array(Token tok)
      : Base(ASTKind::Array, tok) {
  }
};

struct CallFunc : Base {
  ASTPointer callee; // left side, evaluated to be callable object.
  ASTVector args;

  ASTPtr<Function> callee_ast = nullptr;
  builtins::Function const* callee_builtin = nullptr;

  static ASTPtr<CallFunc> New(ASTPointer callee, ASTVector args = {});

  CallFunc(ASTPointer callee, ASTVector args = {})
      : Base(ASTKind::CallFunc, callee->token, callee->token),
        callee(callee),
        args(std::move(args)) {
  }
};

struct Expr : Base {
  Token op;
  ASTPointer lhs;
  ASTPointer rhs;

  static ASTPtr<Expr> New(ASTKind kind, Token optok, ASTPointer lhs,
                          ASTPointer rhs);

  Expr(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs)
      : Base(kind, optok),
        op(this->token),
        lhs(lhs),
        rhs(rhs) {
    this->is_expr = true;
  }
};

struct Block : Base {
  ASTVector list;

  static ASTPtr<Block> New(Token tok, ASTVector list = {});

  Block(Token tok /* "{" */, ASTVector list = {})
      : Base(ASTKind::Block, tok),
        list(std::move(list)) {
  }
};

struct VarDef : Named {
  ASTPtr<TypeName> type;
  ASTPointer init;

  static ASTPtr<VarDef> New(Token tok, Token name, ASTPtr<TypeName> type,
                            ASTPointer init);

  static ASTPtr<VarDef> New(Token tok, Token name) {
    return New(tok, name, nullptr, nullptr);
  }

  VarDef(Token tok, Token name, ASTPtr<TypeName> type = nullptr,
         ASTPointer init = nullptr)
      : Named(ASTKind::Vardef, tok, name),
        type(type),
        init(init) {
  }
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

  struct For {
    ASTVector init;
    ASTPointer cond;
    ASTPtr<Block> block;
  };

  struct TryCatch {
    ASTPtr<Block> tryblock;

    Token varname; // name of variable to catch exception instance
    ASTPtr<Block> catched;
  };

  template <class T>
  T get_data() const {
    return std::any_cast<T>(this->_astdata);
  }

  void set_data(auto data) {
    this->_astdata = data;
  }

  ASTPtr<AST::Expr> get_expr() const {
    return ASTCast<AST::Expr>(this->get_data<ASTPointer>());
  }

  static ASTPtr<Statement> NewIf(Token tok, ASTPointer cond,
                                 ASTPointer if_true,
                                 ASTPointer if_false = nullptr);

  static ASTPtr<Statement> NewSwitch(Token tok, ASTPointer cond,
                                     std::vector<Switch::Case> cases = {});

  static ASTPtr<Statement> NewFor(Token tok, ASTVector init,
                                  ASTPointer cond, ASTPtr<Block> block);

  static ASTPtr<Statement> NewTryCatch(Token tok, ASTPtr<Block> tryblock,
                                       Token vname, ASTPtr<Block> catched);

  static ASTPtr<Statement> New(ASTKind kind, Token tok,
                               std::any data = (ASTPointer) nullptr);

  Statement(ASTKind kind, Token tok, std::any data)
      : Base(kind, tok),
        _astdata(data) {
  }

private:
  std::any _astdata;
};

struct Templatable : Named {
  Token tok_template;

  bool is_templated = false;

  ASTVec<TypeName> template_params;

protected:
  using Named::Named;
};

struct TypeName : Named {
  ASTVector type_params;
  bool is_const;

  TypeInfo type;
  ASTPtr<Class> ast_class;

  static ASTPtr<TypeName> New(Token nametok);

  TypeName(Token name)
      : Named(ASTKind::TypeName, name),
        is_const(false) {
  }
};

struct Function : Templatable {
  struct Argument {
    Token name;
    ASTPtr<TypeName> type;

    std::string get_name() const {
      return this->name.str;
    }

    Argument(Token name, ASTPtr<TypeName> type = nullptr)
        : name(name),
          type(type) {
    }
  };

  std::vector<Argument> arguments;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  bool is_var_arg;

  Argument& add_arg(Token const& tok, ASTPtr<TypeName> type = nullptr) {
    return this->arguments.emplace_back(tok, type);
  }

  Argument* find_arg(std::string const& name) {
    for (auto&& arg : this->arguments)
      if (arg.get_name() == name)
        return &arg;

    return nullptr;
  }

  static ASTPtr<Function> New(Token tok, Token name);

  static ASTPtr<Function> New(Token tok, Token name,
                              std::vector<Argument> args, bool is_var_arg,
                              ASTPtr<TypeName> rettype,
                              ASTPtr<Block> block);

  Function(Token tok, Token name)
      : Templatable(ASTKind::Function, tok, name),
        return_type(nullptr),
        block(nullptr),
        is_var_arg(false) {
  }

  Function(Token tok, Token name, std::vector<Argument> args,
           bool is_var_arg, ASTPtr<TypeName> rettype, ASTPtr<Block> block)
      : Templatable(ASTKind::Function, tok, name),
        arguments(args),
        return_type(rettype),
        block(block),
        is_var_arg(is_var_arg) {
  }
};

struct Enum : Templatable {
  ASTPtr<Block> enumerators; // ->list = ASTVec<Variable>

  ASTPointer& append(Token name /* , todo: typename*/) {
    return this->enumerators->list.emplace_back(Variable::New(name));
  }

  static ASTPtr<Enum> New(Token tok, Token name);

  Enum(Token tok, Token name)
      : Templatable(ASTKind::Enum, tok, name),
        enumerators(Block::New("")) {
  }
};

struct Class : Templatable {
  ASTPtr<Block> block;
  ASTPtr<Function> constructor = nullptr;

  ASTPointer& append(ASTPointer ast) {
    return this->block->list.emplace_back(ast);
  }

  ASTVec<VarDef> get_member_variables() {
    ASTVec<VarDef> v;

    for (auto&& x : this->block->list)
      if (x->kind == ASTKind::Vardef)
        v.emplace_back(ASTCast<VarDef>(x));

    return v;
  }

  ASTVec<Function> get_member_functions() {
    ASTVec<Function> v;

    for (auto&& x : this->block->list)
      if (x->kind == ASTKind::Function)
        v.emplace_back(ASTCast<Function>(x));

    return v;
  }

  static ASTPtr<Class> New(Token tok, Token name);

  static ASTPtr<Class> New(Token tok, Token name,
                           ASTPtr<Block> definitions);

  Class(Token tok, Token name)
      : Templatable(ASTKind::Class, tok, name),
        block(Block::New("")) {
  }
};

enum ASTWalkerLocation {
  AW_Begin,
  AW_End,
};

void walk_ast(ASTPointer ast,
              std::function<std::any(ASTWalkerLocation, ASTPointer)> fn);

} // namespace AST

} // namespace fire
