#pragma once

namespace fire::AST {

struct Value : Base {
  ObjPointer value;

  static ASTPtr<Value> New(Token const& tok, ObjPointer val) {
    return ASTNew<Value>(tok, val);
  }

  ASTPointer Clone() const override {
    return Value::New(this->token, this->value);
  }

  Value(Token const& tok, ObjPointer value)
      : Base(ASTKind::Value, tok),
        value(value) {
  }
};

struct Identifier : Named {
  Token paramtok; // "<"

  ASTVector id_params; // T = Identifier or ScopeResol

  // for Kind::FuncName or BuiltinFuncName
  ASTVec<Function> candidates;
  vector<builtins::Function const*> candidates_builtin;

  bool template_func_decided = false;
  ASTPtr<Function> template_original;

  TypeInfo ft_ret;
  vector<TypeInfo> ft_args;

  vector<TypeInfo> template_args;

  //
  // for BuiltinMemberVariable
  builtins::MemberVariable const* blt_member_var = nullptr;

  //
  //----------------------

  //
  // for Kind::Variable
  int distance = 0;
  int index = 0; // (=> or member variable, enumerator)
  int index_add = 0;
  sema::LocalVar* lvar_ptr = nullptr;

  //
  //----------------------

  int digging_depth = 0; // when accessing to member-variable in base-class

  ASTPtr<Class> ast_class = nullptr;
  ASTPtr<Enum> ast_enum = nullptr;

  static ASTPtr<Identifier> New(Token tok) {
    return ASTNew<Identifier>(tok);
  }

  ASTPointer Clone() const override {
    auto x = New(this->token);

    for (auto&& p : this->id_params)
      x->id_params.emplace_back(p->Clone());

    x->template_func_decided = this->template_func_decided;

    x->template_original = this->template_original;

    x->ft_ret = this->ft_ret;
    x->ft_args = this->ft_args;

    x->template_args = this->template_args;

    x->distance = this->distance;
    x->index = this->index;
    x->index_add = this->index_add;
    x->lvar_ptr = this->lvar_ptr;

    x->ast_class = this->ast_class;
    x->ast_enum = this->ast_enum;

    return x;
  }

  Identifier* GetID() override {
    return this;
  }

  Identifier(Token t)
      : Named(ASTKind::Identifier, t, t) {
  }
};

struct ScopeResol : Named {
  ASTPtr<Identifier> first;
  ASTVec<Identifier> idlist;

  static ASTPtr<ScopeResol> New(ASTPtr<Identifier> first) {
    return ASTNew<ScopeResol>(first);
  }

  ASTPointer Clone() const override {
    auto x = New(ASTCast<AST::Identifier>(this->first->Clone()));

    for (auto&& id : idlist)
      x->idlist.emplace_back(ASTCast<AST::Identifier>(id));

    return x;
  }

  Identifier* GetID() override {
#if _DBG_DONT_USE_SMART_PTR_
    return *idlist.rbegin();
#else
    return idlist.rbegin()->get();
#endif
  }

  ASTPtr<Identifier> GetLastID() const {
    return *this->idlist.rbegin();
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

  ASTPointer Clone() const override;

  Array(Token tok);
};

struct CallFunc : Base {
  ASTPointer callee; // left side, evaluated to be callable object.
  ASTVector args;

  ASTPtr<Function> callee_ast = nullptr;
  builtins::Function const* callee_builtin = nullptr;

  bool call_functor = false;

  ASTPtr<Enum> ast_enum = nullptr;
  size_t enum_index = 0;

  bool IsMemberCall = false;

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

} // namespace fire::AST