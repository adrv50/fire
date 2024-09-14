#include "AST.h"
#include "alert.h"

namespace metro::AST {

template <class T, class... Args>
requires std::constructible_from<T, Args...>
ASTPtr<T> ASTNew(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

i64 Base::GetChilds(ASTVector& out) const {

  using K = ASTKind;

  if (this->is_expr) {
    auto x = this->As<Expr>();

    out.emplace_back(x->lhs);
    out.emplace_back(x->rhs);

    return 2;
  }

  todo_impl;
  switch (this->kind) {}

  return (i64)out.size();
}

ASTPtr<Value> Value::New(Token tok, ObjPointer val) {
  return ASTNew<Value>(tok, val);
}

ASTPtr<Variable> Variable::New(Token tok) {
  return ASTNew<Variable>(tok);
}

ASTPtr<CallFunc> CallFunc::New(ASTPointer expr, ASTVector args) {
  return ASTNew<CallFunc>(expr, std::move(args));
}

ASTPtr<Expr> Expr::New(ASTKind kind, Token op, ASTPointer lhs,
                       ASTPointer rhs) {
  return ASTNew<Expr>(kind, op, lhs, rhs);
}

ASTPtr<Block> Block::New(Token tok, ASTVector list) {
  return ASTNew<Block>(tok, std::move(list));
}

ASTPtr<VarDef> VarDef::New(ASTPtr<TypeName> type, ASTPointer init) {
  return ASTNew<VarDef>(Token(""), Token(""), type, init);
}

ASTPtr<VarDef> VarDef::New(Token tok, Token name,
                           ASTPtr<TypeName> type, ASTPointer init) {
  return ASTNew<VarDef>(tok, name, type, init);
}

ASTPtr<Statement> Statement::NewIf(Token tok, ASTPointer cond,
                                   ASTPointer if_true,
                                   ASTPointer if_false) {
  return ASTNew<Statement>(ASTKind::If, tok,
                           If{cond, if_true, if_false});
}

ASTPtr<Statement>
Statement::NewSwitch(Token tok, ASTPointer cond,
                     std::vector<Switch::Case> cases) {
  return ASTNew<Statement>(ASTKind::Switch, tok,
                           Switch{cond, std::move(cases)});
}

ASTPtr<Statement> Statement::NewFor(Token tok, ASTVector init,
                                    ASTPointer cond, ASTVector count,
                                    ASTPtr<Block> block) {

  return ASTNew<Statement>(
      ASTKind::For, tok,
      For{std::move(init), cond, std::move(count), block});
}

ASTPtr<TypeName> TypeName::New(Token nametok) {
  return ASTNew<TypeName>(std::move(nametok));
}

} // namespace metro::AST