#include "AST.h"
#include "alert.h"

namespace fire::AST {

template <class T, class... Args>
requires std::constructible_from<T, Args...>
ASTPtr<T> ASTNew(Args&&... args) {
#if _DBG_DONT_USE_SMART_PTR_
  return new T(std::forward<Args>(args)...);
#else
  return std::make_shared<T>(std::forward<Args>(args)...);
#endif
}

Expr const* Base::as_expr() const {
  return static_cast<Expr const*>(this);
}

Statement const* Base::as_stmt() const {
  return static_cast<Statement const*>(this);
}

Expr* Base::as_expr() {
  return static_cast<Expr*>(this);
}

Statement* Base::as_stmt() {
  return static_cast<Statement*>(this);
}

i64 Base::GetChilds(ASTVector& out) const {

  (void)out;
  todo_impl;

  return 0;
}

ASTPtr<Value> Value::New(Token tok, ObjPointer val) {
  return ASTNew<Value>(tok, val);
}

ASTPtr<Variable> Variable::New(Token tok, int index, int backstep) {
  return ASTNew<Variable>(tok, index, backstep);
}

ASTPtr<Identifier> Identifier::New(Token tok) {
  return ASTNew<Identifier>(tok);
}

ASTPtr<ScopeResol> ScopeResol::New(ASTPtr<Identifier> first) {
  return ASTNew<ScopeResol>(first);
}

ASTPtr<Array> Array::New(Token tok) {
  return ASTNew<Array>(tok);
}

ASTPtr<CallFunc> CallFunc::New(ASTPointer expr, ASTVector args) {
  return ASTNew<CallFunc>(expr, std::move(args));
}

ASTPtr<Expr> Expr::New(ASTKind kind, Token op, ASTPointer lhs, ASTPointer rhs) {
  return ASTNew<Expr>(kind, op, lhs, rhs);
}

ASTPtr<Block> Block::New(Token tok, ASTVector list) {
  return ASTNew<Block>(tok, std::move(list));
}

ASTPtr<VarDef> VarDef::New(Token tok, Token name, ASTPtr<TypeName> type,
                           ASTPointer init) {
  return ASTNew<VarDef>(tok, name, type, init);
}

ASTPtr<Statement> Statement::NewIf(Token tok, ASTPointer cond, ASTPointer if_true,
                                   ASTPointer if_false) {
  return ASTNew<Statement>(ASTKind::If, tok, If{cond, if_true, if_false});
}

ASTPtr<Statement> Statement::NewSwitch(Token tok, ASTPointer cond,
                                       std::vector<Switch::Case> cases) {
  return ASTNew<Statement>(ASTKind::Switch, tok, Switch{cond, std::move(cases)});
}

ASTPtr<Statement> Statement::NewFor(Token tok, ASTVector init, ASTPointer cond,
                                    ASTPtr<Block> block) {

  return ASTNew<Statement>(ASTKind::For, tok, For{std::move(init), cond, block});
}

ASTPtr<Statement> Statement::NewTryCatch(Token tok, ASTPtr<Block> tryblock, Token vname,
                                         ASTPtr<Block> catched) {
  return ASTNew<Statement>(ASTKind::TryCatch, tok, TryCatch{tryblock, vname, catched});
}

ASTPtr<Statement> Statement::New(ASTKind kind, Token tok, std::any data) {
  return ASTNew<Statement>(kind, tok, std::move(data));
}

ASTPtr<TypeName> TypeName::New(Token nametok) {
  return ASTNew<TypeName>(nametok);
}

ASTPtr<Argument> Argument::New(Token nametok, ASTPtr<TypeName> type) {
  return ASTNew<Argument>(nametok, type);
}

ASTPtr<Function> Function::New(Token tok, Token name) {
  return ASTNew<Function>(tok, name);
}

ASTPtr<Function> Function::New(Token tok, Token name, ASTVec<Argument> args,
                               bool is_var_arg, ASTPtr<TypeName> rettype,
                               ASTPtr<Block> block) {
  return ASTNew<Function>(tok, name, std::move(args), is_var_arg, rettype, block);
}

ASTPtr<Enum> Enum::New(Token tok, Token name) {
  return ASTNew<Enum>(tok, name);
}

ASTPtr<Class> Class::New(Token tok, Token name) {
  return ASTNew<Class>(tok, name);
}

ASTPtr<Class> Class::New(Token tok, Token name, ASTPtr<Block> definitions) {
  auto ast = ASTNew<Class>(tok, name);

  ast->block = definitions;

  return ast;
}

ASTPointer Identifier::Clone() const {
  auto x = New(this->token);

  x->paramtok = this->paramtok;

  for (auto&& p : this->id_params)
    x->id_params.emplace_back(ASTCast<TypeName>(p->Clone()));

  return x;
}

ASTPointer TypeName::Clone() const {
  auto x = New(this->name);

  for (auto&& p : this->type_params)
    x->type_params.emplace_back(ASTCast<TypeName>(p->Clone()));

  x->is_const = this->is_const;

  x->type = this->type;

  if (this->ast_class)
    x->ast_class = ASTCast<AST::Class>(this->ast_class->Clone());

  return x;
}

ASTPointer VarDef::Clone() const {
  return New(this->token, this->name,
             ASTCast<TypeName>(this->type ? this->type->Clone() : nullptr),
             this->init ? this->init->Clone() : nullptr);
}

ASTPointer Statement::Clone() const {
  switch (this->kind) {
  case ASTKind::If:
    return NewIf(this->token, this->get_data<If>().cond, this->get_data<If>().if_true,
                 this->get_data<If>().if_false);

  case ASTKind::Switch: {
    auto d = this->get_data<Switch>();
    std::vector<Switch::Case> _cases;

    for (auto&& c : d.cases)
      _cases.emplace_back(
          Switch::Case{.expr = c.expr->Clone(), .block = c.block->Clone()});

    return NewSwitch(this->token, d.cond->Clone(), std::move(_cases));
  }

  case ASTKind::For:
    todo_impl;

  case ASTKind::Break:
  case ASTKind::Continue:
    return New(this->kind, this->token, nullptr);

  case ASTKind::Throw:
  case ASTKind::Return:
    break;

  default:
    todo_impl;
  }

  return New(this->kind, this->token, this->get_expr()->Clone());
}

} // namespace fire::AST