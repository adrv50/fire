#include "alert.h"
#include "AST.h"
#include "ASTWalker.h"

namespace fire::AST {

void walk_ast(ASTPointer ast, std::function<void(ASTWalkerLocation, ASTPointer)> fn) {

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
    if (ast->_constructed_as == Kind::ScopeResol)
      goto _label_scope_resol;

  case Kind::Identifier:
    for (auto&& id_param : ASTCast<AST::Identifier>(ast)->id_params)
      walk_ast(id_param, fn);

    break;

  _label_scope_resol:
  case Kind::ScopeResol: {
    auto x = ASTCast<AST::ScopeResol>(ast);

    walk_ast(x->first, fn);

    for (auto&& id : x->idlist)
      walk_ast(id, fn);

    break;
  }

  case Kind::CallFunc: {
    auto x = ASTCast<AST::CallFunc>(ast);

    walk_ast(x->callee, fn);

    for (auto&& arg : x->args)
      walk_ast(arg, fn);

    break;
  }

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
    auto x = ast->As<AST::Statement>()->data_if;

    walk_ast(x->cond, fn);
    walk_ast(x->if_true, fn);
    walk_ast(x->if_false, fn);

    break;
  }

  case Kind::Switch:
    todo_impl;

  case Kind::While: {
    auto d = ast->As<AST::Statement>()->data_while;

    walk_ast(d->cond, fn);
    walk_ast(d->block, fn);

    break;
  }

  case Kind::Break:
  case Kind::Continue:
    break;

  case Kind::Return:
  case Kind::Throw:
    walk_ast(ast->As<AST::Statement>()->expr, fn);
    break;

  case Kind::TryCatch: {
    auto d = ast->As<AST::Statement>()->data_try_catch;

    walk_ast(d->tryblock, fn);

    for (auto&& c : d->catchers) {
      walk_ast(c.type, fn);
      walk_ast(c.catched, fn);
    }

    break;
  }

  case Kind::Argument: {
    walk_ast(ASTCast<AST::Argument>(ast)->type, fn);
    break;
  }

  case Kind::Function: {
    auto x = ast->As<AST::Function>();

    for (auto&& y : x->arguments)
      walk_ast(y, fn);

    walk_ast(x->return_type, fn);
    walk_ast(x->block, fn);

    break;
  }

  case Kind::TypeName: {
    auto x = ASTCast<AST::TypeName>(ast);

    for (auto&& param : x->type_params)
      walk_ast(param, fn);

    walk_ast(x->ast_class, fn);

    break;
  }

  default:
    alertmsg(static_cast<int>(ast->kind));
    todo_impl;
  }

  fn(AW_End, ast);
}

} // namespace fire::AST