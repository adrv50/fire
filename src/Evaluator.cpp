#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace metro::eval {

Evaluator::Evaluator(AST::Program prg) : root(prg) {
  for (auto&& ast : prg.list) {
    switch (ast->kind) {
    case ASTKind::Function:
      this->functions.emplace_back(ASTCast<AST::Function>(ast));
      break;
    }
  }
}

ObjPointer Evaluator::find_var(std::string_view name) {
  for (i64 i = stack.size() - 1; i >= 0; i--) {
    auto& vartable = this->stack[i];

    if (vartable.map.contains(name))
      return vartable.map[name];
  }

  return nullptr;
}

ASTPtr<Function> Evaluator::find_func(std::string_view name) {
  for (auto&& astfn : this->functions) {
    if (astfn->GetName() == name)
      return astfn;
  }

  return nullptr;
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
    if (pobj)
      return pobj;

    auto func = this->find_func(x->GetName());

    if (func)
      return ObjNew<ObjCallable>(func.get());

    auto bfn = builtin::find_builtin_func(std::string(x->GetName()));

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
      fn_lvar.map[(*fn_arg_it)->GetName()] = this->evaluate(arg);
    }

    this->evaluate(func->block);

    auto ret = fn_lvar.result;
    this->pop();

    return ret;
  }

  case Kind::Function:
  case Kind::Enum:
  case Kind::Class:
    break;
  }

  return nullptr;
}

void Evaluator::do_eval() {
  stack.emplace_back(); // global variable

  for (auto&& x : this->root.list) {
    evaluate(x);
  }

  stack.clear();
}

} // namespace metro::eval