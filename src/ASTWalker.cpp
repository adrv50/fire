#include "alert.h"
#include "AST.h"

namespace fire::AST {

void walk_ast(ASTPointer ast,
              std::function<std::any(ASTWalkerLocation, ASTPointer)> fn) {

  using Kind = ASTKind;

  if (!ast)
    return;

  fn(AW_Begin, ast);

  if (ast->is_expr) {
    walk_ast(ASTCast<AST::Expr>(ast)->lhs, fn);
    walk_ast(ASTCast<AST::Expr>(ast)->rhs, fn);
    return;
  }

  switch (ast->kind) {

  case Kind::Value:
    break;

  case Kind::Variable:
  case Kind::FuncName:
  case Kind::Enumerator:
    if (ast->_constructed_as == Kind::Identifier)
      goto _label_scope_resol;

  case Kind::Identifier:
    break;

  _label_scope_resol:
  case Kind::ScopeResol:
    break;

  case Kind::Array: {
    auto x = ast->As<AST::Array>();

    for (auto&& y : x->elements)
      walk_ast(y, fn);

    break;
  }

  case Kind::Block:
    for (auto&& x : ast->As<AST::Block>()->list)
      walk_ast(x, fn);

    break;

  case Kind::Vardef: {
    auto x = ast->As<AST::VarDef>();

    walk_ast(x->type, fn);
    walk_ast(x->init, fn);

    break;
  }

  case Kind::If: {
    auto x = ast->As<AST::Statement>()->get_data<AST::Statement::If>();

    walk_ast(x.cond, fn);
    walk_ast(x.if_true, fn);
    walk_ast(x.if_false, fn);

    break;
  }

  case Kind::Switch:
  case Kind::For:
    todo_impl;

  case Kind::Break:
  case Kind::Continue:
    break;

  case Kind::Return:
  case Kind::Throw:
    walk_ast(ast->As<AST::Statement>()->get_data<ASTPointer>(), fn);
    break;

  case Kind::TryCatch: {
    auto x =
        ast->As<AST::Statement>()->get_data<AST::Statement::TryCatch>();

    walk_ast(x.tryblock, fn);
    walk_ast(x.catched, fn);

    break;
  }

  case Kind::Function: {
    auto x = ast->As<AST::Function>();

    for (auto&& y : x->arguments)
      walk_ast(y.type, fn);

    walk_ast(x->return_type, fn);
    walk_ast(x->block, fn);

    break;
  }

  default:
    todo_impl;
  }

  fn(AW_End, ast);
}

} // namespace fire::AST