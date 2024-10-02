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

  Array,

  IndexRef,

  MemberAccess,

  // /-----------------
  MemberVariable,
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
  For,

  Break,
  Continue,
  Return,

  Throw,

  TryCatch,

  Argument,
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

  Value const* as_value() const {
    return (Value const*)this;
  }

  virtual Identifier* GetID() {
    return nullptr;
  }

  virtual ASTPointer Clone() const = 0;

  [[maybe_unused]] i64 GetChilds(ASTVector& out) const;

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

  ASTPointer Clone() const override {
    assert(this->value);

    auto xx = New(this->token, this->value->Clone());

    assert(xx->value);

    return xx;
  }

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

// ------------------
// not used
//
struct Variable : Named {
  int index = 0;
  int backstep = 0; // N 個前のスコープに戻る

  static ASTPtr<Variable> New(Token tok, int index = 0, int backstep = 0);

  ASTPointer Clone() const override {
    return New(this->token);
  }

  Variable(Token tok, int index = 0, int backstep = 0)
      : Named(ASTKind::Variable, tok, tok),
        index(index),
        backstep(backstep) {
  }
};
// ------------------

struct Identifier : Named {
  Token paramtok; // "<"

  ASTVector id_params; // T = Identifier or ScopeResol

  //
  // for Kind::FuncName or BuiltinFuncName
  ASTVec<Function> candidates;
  vector<builtins::Function const*> candidates_builtin;
  bool sema_allow_ambigious = false;

  //
  // for BuiltinMemberVariable
  builtins::MemberVariable const* blt_member_var = nullptr;

  //
  // for Kind::Variable
  int depth = 0;
  int index = 0; // (=> also member variable)

  ASTPtr<Class> ast_class = nullptr;
  ASTPtr<Enum> ast_enum = nullptr;

  TypeInfo self_type; // if member

  static ASTPtr<Identifier> New(Token tok);

  ASTPointer Clone() const override;

  Identifier* GetID() override {
    return this;
  }

  Identifier(Token tok)
      : Named(ASTKind::Identifier, tok, tok) {
  }
};

struct ScopeResol : Named {
  ASTPtr<Identifier> first;
  ASTVec<Identifier> idlist;

  static ASTPtr<ScopeResol> New(ASTPtr<Identifier> first);

  ASTPointer Clone() const override {
    auto x = New(ASTCast<AST::Identifier>(this->first->Clone()));

    for (ASTPtr<Identifier> const& id : this->idlist)
      x->idlist.emplace_back(ASTCast<Identifier>(id->Clone()));

    return x;
  }

  Identifier* GetID() override {
    return idlist.rbegin()->get();
  }

  ASTPtr<Identifier> GetLastID() const {
    return *idlist.rbegin();
  }

  ScopeResol(ASTPtr<Identifier> first)
      : Named(ASTKind::ScopeResol, first->token),
        first(first) {
  }
};

struct Array : Base {
  ASTVector elements;

  TypeInfo elem_type; // set in Sema

  static ASTPtr<Array> New(Token tok);

  ASTPointer Clone() const override {
    auto x = New(this->token);

    for (ASTPointer const& e : this->elements)
      x->elements.emplace_back(e->Clone());

    return x;
  }

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

  ASTPointer Clone() const override {
    auto x = New(this->callee);

    for (ASTPointer const& a : this->args)
      x->args.emplace_back(a->Clone());

    x->callee_ast = this->callee_ast;
    x->callee_builtin = this->callee_builtin;

    return x;
  }

  ASTPtr<Class> get_class_ptr() const {
    return this->callee->GetID()->ast_class;
  }

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

  static ASTPtr<Expr> New(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs);

  ASTPointer Clone() const override {
    return New(this->kind, this->op, this->lhs->Clone(), this->rhs->Clone());
  }

  Identifier* GetID() override {
    return this->rhs->GetID();
  }

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
  int stack_size = 0; // count of variable definition

  static ASTPtr<Block> New(Token tok, ASTVector list = {});

  ASTPointer Clone() const override {
    auto x = New(this->token);

    for (ASTPointer const& a : this->list)
      x->list.emplace_back(a->Clone());

    return x;
  }

  Block(Token tok /* "{" */, ASTVector list = {})
      : Base(ASTKind::Block, tok),
        list(std::move(list)) {
  }
};

struct TypeName : Named {
  ASTVector type_params;
  bool is_const;

  TypeInfo type;
  ASTPtr<Class> ast_class = nullptr;

  static ASTPtr<TypeName> New(Token nametok);

  ASTPointer Clone() const override;

  TypeName(Token name)
      : Named(ASTKind::TypeName, name),
        is_const(false) {
  }
};

struct VarDef : Named {
  ASTPtr<TypeName> type;
  ASTPointer init;

  int index = 0;

  static ASTPtr<VarDef> New(Token tok, Token name, ASTPtr<TypeName> type,
                            ASTPointer init);

  static ASTPtr<VarDef> New(Token tok, Token name) {
    return New(tok, name, nullptr, nullptr);
  }

  ASTPointer Clone() const override;

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

  static ASTPtr<Statement> NewIf(Token tok, ASTPointer cond, ASTPointer if_true,
                                 ASTPointer if_false = nullptr);

  static ASTPtr<Statement> NewSwitch(Token tok, ASTPointer cond,
                                     std::vector<Switch::Case> cases = {});

  static ASTPtr<Statement> NewFor(Token tok, ASTVector init, ASTPointer cond,
                                  ASTPtr<Block> block);

  static ASTPtr<Statement> NewTryCatch(Token tok, ASTPtr<Block> tryblock, Token vname,
                                       ASTPtr<Block> catched);

  static ASTPtr<Statement> New(ASTKind kind, Token tok,
                               std::any data = (ASTPointer) nullptr);

  ASTPointer Clone() const override;

  Statement(ASTKind kind, Token tok, std::any data)
      : Base(kind, tok),
        _astdata(data) {
  }

  //
  // when ASTKind::Return
  int ret_func_scope_distance = 0;

private:
  std::any _astdata;
};

struct Templatable : Named {
  Token tok_template;

  bool is_templated = false;

  TokenVector template_param_names;

protected:
  using Named::Named;

  void _Copy(Templatable const* _t) {
    this->tok_template = _t->tok_template;
    this->is_templated = _t->is_templated;

    for (auto&& e : _t->template_param_names) {
      this->template_param_names.emplace_back(e);
    }
  }
};

struct Argument : Named {
  ASTPtr<TypeName> type;

  static ASTPtr<Argument> New(Token nametok, ASTPtr<TypeName> type);

  ASTPointer Clone() const {
    return New(this->name,
               ASTCast<AST::TypeName>(this->type ? this->type->Clone() : nullptr));
  }

  Argument(Token nametok, ASTPtr<TypeName> type)
      : Named(ASTKind::Argument, nametok),
        type(type) {
  }
};

struct Function : Templatable {
  ASTVec<Argument> arguments;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  bool is_var_arg;

  ASTPtr<Class> member_of = nullptr;

  ASTPtr<Argument>& add_arg(Token const& tok, ASTPtr<TypeName> type = nullptr) {
    return this->arguments.emplace_back(Argument::New(tok, type));
  }

  ASTPtr<Argument> find_arg(std::string const& name) {
    for (auto&& arg : this->arguments)
      if (arg->GetName() == name)
        return arg;

    return nullptr;
  }

  static ASTPtr<Function> New(Token tok, Token name);

  static ASTPtr<Function> New(Token tok, Token name, ASTVec<Argument> args,
                              bool is_var_arg, ASTPtr<TypeName> rettype,
                              ASTPtr<Block> block);

  ASTPointer Clone() const override {
    auto x = New(this->token, this->name);

    x->_Copy(this);

    for (auto&& arg : this->arguments) {
      x->arguments.emplace_back(ASTCast<Argument>(arg->Clone()));
    }

    if (this->return_type)
      x->return_type = ASTCast<TypeName>(this->return_type->Clone());

    x->block = ASTCast<Block>(this->block->Clone());
    x->is_var_arg = this->is_var_arg;

    return x;
  }

  Function(Token tok, Token name)
      : Function(tok, name, {}, false, nullptr, nullptr) {
  }

  Function(Token tok, Token name, ASTVec<Argument> args, bool is_var_arg,
           ASTPtr<TypeName> rettype, ASTPtr<Block> block)
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

  ASTPointer Clone() const override {
    auto x = New(this->token, this->name);

    x->_Copy(this);

    x->enumerators = ASTCast<Block>(this->enumerators->Clone());

    return x;
  }

  Enum(Token tok, Token name)
      : Templatable(ASTKind::Enum, tok, name),
        enumerators(Block::New("")) {
  }
};

struct Class : Templatable {
  ASTPtr<Block> block;
  std::weak_ptr<Function> constructor;

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

  static ASTPtr<Class> New(Token tok, Token name, ASTPtr<Block> definitions);

  ASTPointer Clone() const override {
    auto x = New(this->token, this->name, ASTCast<Block>(this->block->Clone()));

    x->_Copy(this);

    for (auto&& y : x->block->list)
      if (y->kind == ASTKind::Function && y->As<Named>()->GetName() == x->GetName()) {
        x->constructor = ASTCast<Function>(y);
        break;
      }

    return x;
  }

  Class(Token tok, Token name)
      : Templatable(ASTKind::Class, tok, name),
        block(Block::New("")) {
  }
};

enum ASTWalkerLocation {
  AW_Begin,
  AW_End,
};

void walk_ast(ASTPointer ast, std::function<void(ASTWalkerLocation, ASTPointer)> fn);

string ToString(ASTPointer ast);

} // namespace AST

} // namespace fire
