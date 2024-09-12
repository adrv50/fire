#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace metro::eval {

using namespace AST;

void Evaluator::do_eval() {
  this->push(); // global variable

  for (auto&& x : this->root.list) {
    evaluate(x);
  }

  this->pop();
}

ObjPointer Evaluator::eval_member_access(ASTPtr<AST::Expr> ast) {

  auto obj = this->evaluate(ast->lhs);
  auto& name = ast->rhs->token.str;

  if (obj->type.kind == TypeKind::Instance) {
    auto inst = PtrCast<ObjInstance>(obj);

    if (inst->member.contains(name))
      return inst->member[name];

    for (auto&& mf : inst->member_funcs) {
      if (mf->GetName() == name)
        return mf;
    }
  }

  if (auto [fn_ast, blt] = this->find_func(name); fn_ast)
    return ObjNew<ObjCallable>(fn_ast);
  else if (blt)
    return ObjNew<ObjCallable>(blt);

  Error(ast->token, "not found the name '" + ast->rhs->token.str +
                        "' in `" + obj->type.to_string() +
                        "` type object.")();
}

ObjPointer Evaluator::evaluate(ASTPointer ast) {
  using Kind = ASTKind;

  switch (ast->kind) {
  case Kind::Value:
    return ASTCast<Value>(ast)->value;

  case Kind::Variable: {
    auto x = ASTCast<Variable>(ast);

    auto pobj = this->find_var(x->GetName());

    // found variable
    if (pobj) {
      if (*pobj)
        return *pobj;

      Error(ast->token, "use variable before assignment")();
    }

    auto [func, bfn] = this->find_func(x->GetName());

    if (func)
      return ObjNew<ObjCallable>(func);

    if (bfn)
      return ObjNew<ObjCallable>(bfn);

    Error(ast->token, "no name defined")();
  }

  case Kind::CallFunc: {
    auto cf = ASTCast<CallFunc>(ast);

    auto funcobj = this->evaluate(cf->expr);

    if (!funcobj->is_callable()) {
      Error(cf->expr->token,
            "`" + funcobj->type.to_string() + "` is not callable")();
    }

    auto cb = funcobj->As<ObjCallable>();

    if (cb->builtin) {
      ObjVector args;

      for (auto&& arg : cf->args)
        args.emplace_back(this->evaluate(arg));

      return cb->builtin->Call(std::move(args));
    }

    auto func = cb->func;
    auto& fn_lvar = this->push();

    for (auto fn_arg_it = func->args.begin(); auto&& arg : cf->args) {
      fn_lvar.map[(*fn_arg_it++)->GetName()] = this->evaluate(arg);
    }

    this->evaluate(func->block);

    auto ret = fn_lvar.result;
    this->pop();

    if (ret)
      return ret;

    break;
  }

  case Kind::MemberAccess: {
    return this->eval_member_access(ASTCast<AST::Expr>(ast));
  }

  case Kind::Block: {
    auto _block = ASTCast<AST::Block>(ast);

    for (auto&& x : _block->list)
      this->evaluate(x);

    break;
  }

  case Kind::Vardef: {
    auto x = ASTCast<AST::VarDef>(ast);

    auto& vartable = this->get_cur_vartable();
    auto& obj = vartable.map[x->GetName()];

    if (x->init) {
      obj = this->evaluate(x->init);
    }

    break;
  }

  case Kind::Function:
  case Kind::Enum:
  case Kind::Class:
    break;

  default:
    if (ast->is_expr)
      return this->eval_expr(ASTCast<AST::Expr>(ast));
  }

  return ObjNew<ObjNone>();
}

Evaluator::Evaluator(AST::Program prg) : root(prg) {
  for (auto&& ast : prg.list) {
    switch (ast->kind) {
    case ASTKind::Function:
      this->functions.emplace_back(ASTCast<AST::Function>(ast));
      break;
    }
  }
}

} // namespace metro::eval