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
  return 0;
}

ASTPtr<Value> Value::New(Token tok, ObjPointer val) {
  return ASTNew<Value>(tok, val);
}

ASTPtr<Variable> Variable::New(Token tok) {
  return ASTNew<Variable>(tok);
}

ASTPtr<Identifier> Identifier::New(Token tok) {
  return ASTNew<Identifier>(tok);
}

ASTPtr<Array> Array::New(Token tok) {
  return ASTNew<Array>(tok);
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

ASTPtr<VarDef> VarDef::New(Token tok, Token name, ASTPtr<TypeName> type,
                           ASTPointer init) {
  return ASTNew<VarDef>(tok, name, type, init);
}

ASTPtr<Statement> Statement::NewIf(Token tok, ASTPointer cond,
                                   ASTPointer if_true,
                                   ASTPointer if_false) {
  return ASTNew<Statement>(ASTKind::If, tok, If{cond, if_true, if_false});
}

ASTPtr<Statement> Statement::NewSwitch(Token tok, ASTPointer cond,
                                       std::vector<Switch::Case> cases) {
  return ASTNew<Statement>(ASTKind::Switch, tok,
                           Switch{cond, std::move(cases)});
}

ASTPtr<Statement> Statement::NewFor(Token tok, ASTVector init,
                                    ASTPointer cond, ASTPtr<Block> block) {

  return ASTNew<Statement>(ASTKind::For, tok,
                           For{std::move(init), cond, block});
}

ASTPtr<Statement> Statement::NewTryCatch(Token tok, ASTPtr<Block> tryblock,
                                         Token vname,
                                         ASTPtr<Block> catched) {
  return ASTNew<Statement>(ASTKind::TryCatch, tok,
                           TryCatch{tryblock, vname, catched});
}

ASTPtr<Statement> Statement::New(ASTKind kind, Token tok, std::any data) {
  return ASTNew<Statement>(kind, tok, std::move(data));
}

ASTPtr<TypeName> TypeName::New(Token nametok) {
  return ASTNew<TypeName>(nametok);
}

ASTPtr<Function> Function::New(Token tok, Token name) {
  return ASTNew<Function>(tok, name);
}

ASTPtr<Function> Function::New(Token tok, Token name,
                               std::vector<Argument> args, bool is_var_arg,
                               ASTPtr<TypeName> rettype,
                               ASTPtr<Block> block) {
  return ASTNew<Function>(tok, name, std::move(args), is_var_arg, rettype,
                          block);
}

ASTPtr<Enum> Enum::New(Token tok, Token name) {
  return ASTNew<Enum>(tok, name);
}

ASTPtr<Class> Class::New(Token tok, Token name) {
  return ASTNew<Class>(tok, name);
}

ASTPtr<Class> Class::New(Token tok, Token name,
                         ASTPtr<Block> definitions) {
  auto ast = ASTNew<Class>(tok, name);

  ast->block = definitions;

  return ast;
}

} // namespace fire::AST