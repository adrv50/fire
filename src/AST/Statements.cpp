#include "AST.h"
#include "alert.h"

namespace fire::AST {

// -------------------------------------
//  VarDef

ASTPtr<VarDef> VarDef::New(Token tok, Token name, ASTPtr<TypeName> type,
                           ASTPointer init) {
  return ASTNew<VarDef>(tok, name, type, init);
}

ASTPointer VarDef::Clone() const {
  return New(this->token, this->name,
             ASTCast<TypeName>(this->type ? this->type->Clone() : nullptr),
             this->init ? this->init->Clone() : nullptr);
}

VarDef::VarDef(Token tok, Token name, ASTPtr<TypeName> type, ASTPointer init)
    : Named(ASTKind::Vardef, tok, name),
      type(type),
      init(init) {
}

// -------------------------------------
//  Statement

ASTPtr<Statement> Statement::NewIf(Token tok, ASTPointer cond, ASTPointer if_true,
                                   ASTPointer if_false) {
  return ASTNew<Statement>(ASTKind::If, tok, new If{cond, if_true, if_false});
}

ASTPtr<Statement> Statement::NewSwitch(Token tok, ASTPointer cond,
                                       Vec<Switch::Case> cases) {
  return ASTNew<Statement>(ASTKind::Switch, tok, new Switch{cond, std::move(cases)});
}

ASTPtr<Statement> Statement::NewWhile(Token tok, ASTPointer cond, ASTPtr<Block> block) {

  return ASTNew<Statement>(ASTKind::While, tok, new While{cond, block});
}

ASTPtr<Statement> Statement::NewTryCatch(Token tok, ASTPtr<Block> tryblock,
                                         vector<TryCatch::Catcher> catchers) {
  return ASTNew<Statement>(ASTKind::TryCatch, tok,
                           new TryCatch{tryblock, std::move(catchers)});
}

ASTPtr<Statement> Statement::New(ASTKind kind, Token tok, void* data) {
  return ASTNew<Statement>(kind, tok, data);
}

ASTPtr<Statement> Statement::NewExpr(ASTKind kind, Token tok, ASTPointer expr) {
  return ASTNew<Statement>(kind, tok, expr);
}

ASTPointer Statement::Clone() const {
  switch (this->kind) {
  case ASTKind::If:
    return NewIf(this->token, this->data_if->cond, this->data_if->if_true,
                 this->data_if->if_false);

  case ASTKind::Switch: {
    auto d = this->data_switch;

    Vec<Switch::Case> _cases;

    for (auto&& c : d->cases)
      _cases.emplace_back(
          Switch::Case{.expr = c.expr->Clone(), .block = c.block->Clone()});

    return NewSwitch(this->token, d->cond->Clone(), std::move(_cases));
  }

  case ASTKind::While: {
    auto d = this->data_while;

    return NewWhile(this->token, d->cond->Clone(),
                    ASTCast<AST::Block>(d->block->Clone()));
  }

  case ASTKind::Break:
  case ASTKind::Continue:
    return New(this->kind, this->token, nullptr);

  case ASTKind::Throw:
  case ASTKind::Return:
    break;

  default:
    todo_impl;
  }

  return NewExpr(this->kind, this->token, this->expr->Clone());
}

Statement::Statement(ASTKind kind, Token tok, void* data)
    : Base(kind, tok),
      _data(data) {
}

Statement::Statement(ASTKind kind, Token tok, ASTPointer expr)
    : Base(kind, tok),
      expr(expr) {
}

Statement::~Statement() {
  switch (this->kind) {
  case ASTKind::If:
    delete this->data_if;
    break;

  case ASTKind::Switch:
    delete this->data_switch;
    break;

  case ASTKind::While:
    delete this->data_while;
    break;

  case ASTKind::TryCatch:
    delete this->data_try_catch;
    break;
  }
}

// -------------------------------------
//  Match

ASTPointer Match::Clone() const {
  auto ast = New(this->token, this->cond->Clone(), {});

  for (auto&& p : this->patterns)
    ast->patterns.emplace_back(p.type, p.expr->Clone(),
                               ASTCast<AST::Block>(p.block->Clone()), p.everything,
                               p.is_eval_expr);

  return ast;
}

ASTPtr<Match> Match::New(Token tok, ASTPointer cond, Vec<Pattern> patterns) {
  return ASTNew<Match>(tok, cond, std::move(patterns));
}

Match::Match(Token tok, ASTPointer cond, Vec<Pattern> patterns)
    : Base(ASTKind::Match, tok),
      cond(cond),
      patterns(std::move(patterns)) {
}

} // namespace fire::AST