#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace metro::eval {

using namespace AST;

void Evaluator::do_eval() {
  this->PushStack(); // global variable

  for (auto&& x : this->root->list) {
    evaluate(x);
  }

  this->PopStack();
}

ObjPointer Evaluator::eval_member_access(ASTPtr<AST::Expr> ast) {

  auto obj = this->evaluate(ast->lhs);
  auto& name = ast->rhs->token.str;

  //
  // ObjInstance
  if (obj->is_instance()) {
    auto inst = PtrCast<ObjInstance>(obj);

    if (inst->member.contains(name)) {
      auto ret = inst->member[name];

      if (ret)
        return ret->Clone();

      Error(ast->rhs->token,
            "use member variable before assignment")();
    }

    for (auto&& mf : inst->member_funcs) {
      if (mf->GetName() == name)
        return mf;
    }
  }

  //
  // ObjModule
  if (obj->type.kind == TypeKind::Module) {
    alert;

    auto mod = PtrCast<ObjModule>(obj);

    if (mod->variables.contains(name)) {
      alertmsg(name);
      return mod->variables[name];
    }

    for (auto&& objtype : mod->types) {
      if (objtype->GetName() == name)
        return objtype;
    }

    for (auto&& objfunc : mod->functions) {
      alert;

      if (objfunc->GetName() == name)
        return objfunc;
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

ObjPointer& Evaluator::eval_as_writable(ASTPointer ast) {
  using Kind = ASTKind;

  switch (ast->kind) {
  case Kind::Variable: {
    auto x = ast->As<AST::Variable>();
    auto pvar = this->find_var(x->GetName());

    if (!pvar) {
      Error(ast->token, "undefined variable name")();
    }

    return *pvar;
  }

  case Kind::IndexRef: {
    auto x = ast->As<AST::Expr>();

    auto& obj = this->eval_as_writable(x->lhs);
    auto index = this->evaluate(x->rhs);

    todo_impl;
  }

  case Kind::MemberAccess: {
    auto x = ast->As<AST::Expr>();
    auto& obj = this->eval_as_writable(x->lhs);
    auto name = x->rhs->token.str;

    if (obj->type.kind == TypeKind::Instance) {
      auto inst = obj->As<ObjInstance>();

      if (inst->member.contains(name))
        return inst->member[name];

      Error(x->rhs->token, "class '" + inst->ast->GetName() +
                               "' don't have member '" + name +
                               "'")();
    }

    todo_impl; // find in enum
  }
  }

  Error(ast, "cannot writable")();
}

ObjPointer Evaluator::evaluate(ASTPointer ast) {
  using Kind = ASTKind;

  if (!ast)
    return ObjNew<ObjNone>();

  switch (ast->kind) {
  case Kind::Value:
    return ASTCast<Value>(ast)->value;

  case Kind::Variable: {
    auto x = ASTCast<Variable>(ast);
    auto name = x->GetName();

    auto pobj = this->find_var(name);

    // found variable
    if (pobj) {
      if (*pobj)
        return *pobj;

      Error(ast->token, "use variable before assignment")();
    }

    // find function
    if (auto [func, bfn] = this->find_func(name); func) {
      return ObjNew<ObjCallable>(func);
    }
    else if (bfn) {
      // builtin-func
      return ObjNew<ObjCallable>(bfn);
    }

    // find class
    if (auto C = this->find_class(name); C) {
      auto obj = ObjNew<ObjType>();

      obj->ast_class = C;

      return obj;
    }

    Error(ast->token, "no name defined")();
  }

  case Kind::CallFunc: {
    auto cf = ASTCast<CallFunc>(ast);

    auto obj = this->evaluate(cf->expr);

    ObjVector args;

    for (auto&& x : cf->args) {
      args.emplace_back(
          this->evaluate(x->kind == ASTKind::SpecifyArgumentName
                             ? x->As<AST::Expr>()->rhs
                             : x));
    }

    // type
    if (obj->type.kind == TypeKind::TypeName) {
      auto typeobj = obj->As<ObjType>();

      // class --> make instance
      if (typeobj->ast_class) {
        auto inst = this->new_class_instance(typeobj->ast_class);

        if (inst->have_constructor()) {
          args.insert(args.begin(), inst);

          this->call_function_ast(true, inst->get_constructor(), cf,
                                  args);
        }
        else if (args.size() >= 1) {
          Error(
              cf->args[0]->token,
              "class '" + typeobj->ast_class->GetName() +
                  "' don't have constructor, but given arguments.")();
        }

        return inst;
      }
    }

    // not callable object --> error
    if (!obj->is_callable()) {
      Error(cf->expr->token,
            "`" + obj->type.to_string() + "` is not callable")();
    }

    auto cb = obj->As<ObjCallable>();

    // builtin func
    if (cb->builtin) {
      return cb->builtin->Call(std::move(args));
    }

    return this->call_function_ast(false, cb->func, cf, args);
  }

  case Kind::MemberAccess: {
    return this->eval_member_access(ASTCast<AST::Expr>(ast));
  }

  case Kind::Assign: {
    auto x = ASTCast<AST::Expr>(ast);

    return this->eval_as_writable(x->lhs) = this->evaluate(x->rhs);
  }

  case Kind::Block: {
    auto _block = ASTCast<AST::Block>(ast);
    auto& stack = this->GetCurrentStack();

    for (auto&& x : _block->list) {
      this->evaluate(x);

      if (stack.is_returned)
        break;

      if (this->loop_stack.size())
        if (auto& L = this->GetCurrentLoop();
            L.is_breaked || L.is_continued)
          break;
    }

    break;
  }

  case Kind::Vardef: {
    auto x = ASTCast<AST::VarDef>(ast);
    auto name = x->GetName();

    auto& stack = this->GetCurrentStack();

    auto pvar = stack.find_variable(name);

    if (!pvar)
      pvar = &stack.append(name);

    if (x->init)
      pvar->value = this->evaluate(x->init);

    break;
  }

  case Kind::Return: {
    auto& stack = this->GetCurrentStack();

    stack.result = this->evaluate(std::any_cast<ASTPointer>(
        ast->As<AST::Statement>()->astdata));

    stack.is_returned = true;

    break;
  }

  case Kind::Break:
  case Kind::Continue:
    if (this->loop_stack.empty())
      Error(ast->token, "cannot use in out of loop statement")();

    (ast->kind == Kind::Break ? this->GetCurrentLoop().is_breaked
                              : this->GetCurrentLoop().is_continued) =
        true;

    break;

  case Kind::If: {
    auto x = ast->As<AST::Statement>();
    auto data = std::any_cast<Statement::If>(x->astdata);

    auto cond = this->evaluate(data.cond);

    if (cond->As<ObjPrimitive>()->vb)
      this->evaluate(data.if_true);
    else if (data.if_false)
      this->evaluate(data.if_false);

    break;
  }

  case Kind::Function:
  case Kind::Enum:
  case Kind::Class:
    break;

  default:
    if (ast->is_expr)
      return this->eval_expr(ASTCast<AST::Expr>(ast));

    todo_impl;
  }

  return ObjNew<ObjNone>();
}

Evaluator::Evaluator(ASTPtr<AST::Program> prg)
    : root(prg) {
  for (auto&& ast : prg->list) {
    switch (ast->kind) {
    case ASTKind::Function:
      this->functions.emplace_back(ASTCast<AST::Function>(ast));
      break;

    case ASTKind::Class:
      this->classes.emplace_back(ASTCast<AST::Class>(ast));
      break;
    }
  }
}

} // namespace metro::eval