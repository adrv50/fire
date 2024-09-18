#pragma once

#include <any>
#include <algorithm>
#include <concepts>
#include <string>
#include <fstream>

#include "Object.h"
#include "Token.h"

namespace fire {

enum class ASTKind {
  Value,
  Variable,

  IndexRef,
  MemberAccess,
  CallFunc,

  SpecifyArgumentName, // func(a: 2, b: "aiue")

  Not, // !

  Mul,
  Div,
  Add,
  Sub,
  LShift,
  RShift,
  Bigger,        // a > b
  BiggerOrEqual, // a >= b
  Equal,

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

  // FuncArg,
  Function,

  Enumerator,
  Enum,

  Class,

  Module,

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

  i64 GetChilds(ASTVector& out) const;

  virtual std::string_view GetSourceView() const {
    return token.sourceloc.GetLine();
  }

  virtual ~Base() = default;

protected:
  Base(ASTKind kind, Token token)
      : kind(kind),
        token(token),
        endtok(token) {
  }

  Base(ASTKind kind, Token token, Token endtok)
      : kind(kind),
        token(token),
        endtok(endtok) {
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
  static ASTPtr<Variable> New(Token tok);

  Variable(Token tok)
      : Named(ASTKind::Variable, tok, tok) {
  }
};

struct CallFunc : Base {
  ASTPointer expr; // left side
  ASTVector args;

  static ASTPtr<CallFunc> New(ASTPointer expr, ASTVector args = {});

  CallFunc(ASTPointer expr, ASTVector args = {})
      : Base(ASTKind::CallFunc, expr->token, expr->token),
        expr(expr),
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

  static ASTPtr<VarDef> New(Token tok, Token name,
                            ASTPtr<TypeName> type, ASTPointer init);

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
    ASTVector count;
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

  static ASTPtr<Statement>
  NewSwitch(Token tok, ASTPointer cond,
            std::vector<Switch::Case> cases = {});

  static ASTPtr<Statement> NewFor(Token tok, ASTVector init,
                                  ASTPointer cond, ASTVector count,
                                  ASTPtr<Block> block);

  static ASTPtr<Statement> NewTryCatch(Token tok,
                                       ASTPtr<Block> tryblock,
                                       Token vname,
                                       ASTPtr<Block> catched);

  static ASTPtr<Statement> New(ASTKind kind, Token tok,
                               std::any data = (ASTPointer) nullptr);

  Statement(ASTKind kind, Token tok, std::any data)
      : Base(kind, tok),
        _astdata(data) {
  }

private:
  std::any _astdata;
};

struct TypeName : Named {
  ASTVector type_params;
  ASTVec<TypeName> scope_resol;
  bool is_const;

  TypeInfo type;
  ASTPtr<Class> ast_class;

  static ASTPtr<TypeName> New(Token nametok);

  TypeName(Token name)
      : Named(ASTKind::TypeName, name),
        is_const(false) {
  }
};

struct Function : Named {
  TokenVector arg_names;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  bool is_var_arg;

  Token& add_arg(Token const& tok) {
    return this->arg_names.emplace_back(tok);
  }

  static ASTPtr<Function> New(Token tok, Token name);

  static ASTPtr<Function> New(Token tok, Token name,
                              TokenVector arg_names, bool is_var_arg,
                              ASTPtr<TypeName> rettype,
                              ASTPtr<Block> block);

  Function(Token tok, Token name)
      : Named(ASTKind::Function, tok, name),
        return_type(nullptr),
        block(nullptr),
        is_var_arg(false) {
  }

  Function(Token tok, Token name, TokenVector args, bool is_var_arg,
           ASTPtr<TypeName> rettype, ASTPtr<Block> block)
      : Named(ASTKind::Function, tok, name),
        arg_names(args),
        return_type(rettype),
        block(block),
        is_var_arg(is_var_arg) {
  }
};

struct Enum : Named {
  ASTPtr<Block> enumerators; // ->list = ASTVec<Variable>

  ASTPointer& append(Token name /* , todo: typename*/) {
    return this->enumerators->list.emplace_back(Variable::New(name));
  }

  static ASTPtr<Enum> New(Token tok, Token name);

  Enum(Token tok, Token name)
      : Named(ASTKind::Enum, tok, name) {
  }
};

struct Class : Named {
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
      : Named(ASTKind::Class, tok, name),
        block(Block::New("")) {
  }
};

struct Program : Base {
  ASTVector list;

  static ASTPtr<Program> New();

  Program()
      : Base(ASTKind::Program, {}) {
  }
};

} // namespace AST

} // namespace fire
