#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace metro::eval {

using namespace AST;

template <class T>
using ObjPair = std::pair<ObjPtr<T>, ObjPtr<T>>;

using PrimitiveObjPair = ObjPair<ObjPrimitive>;

template <class T, class U>
static inline ObjPair<T> pair_ptr_cast(std::shared_ptr<U> a,
                                       std::shared_ptr<U> b) {
  static_assert(!std::is_same_v<T, U>);
  return std::make_pair(PtrCast<T>(a), PtrCast<T>(b));
}

template <class T>
static inline PrimitiveObjPair to_primitive_pair(ObjPtr<T> a,
                                                 ObjPtr<T> b) {
  return pair_ptr_cast<ObjPrimitive>(a, b);
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

static void adjust_numeric_type_object(ObjPtr<ObjPrimitive> left,
                                       ObjPtr<ObjPrimitive> right) {

  auto lk = left->type.kind, rk = right->type.kind;

  if (lk == rk)
    return;

  if (left->is_float() != right->is_float()) {
    (left->is_float() ? left : right)->to_float();
    return;
  }

  if (lk == TypeKind::Size)
    right->vsize = (value_type::Size)right->vi;
  else
    left->vsize = (value_type::Size)left->vi;
}

ObjPointer Evaluator::eval_expr(ASTPtr<AST::Expr> ast) {
  using Kind = ASTKind;

  auto lhs = this->evaluate(ast->lhs);
  auto rhs = this->evaluate(ast->rhs);

  switch (ast->kind) {

  case Kind::Add: {

    if (lhs->is_numeric() && rhs->is_numeric()) {
      adjust_numeric_type_object(PtrCast<ObjPrimitive>(lhs),
                                 PtrCast<ObjPrimitive>(rhs));

      auto [lv, rv] = to_primitive_pair(lhs, rhs);

      if (lv->is_float())
        return ObjNew<ObjPrimitive>(lv->vf + rv->vf);

      if (lv->is_size())
        return ObjNew<ObjPrimitive>(lv->vsize + rv->vsize);

      return ObjNew<ObjPrimitive>(lv->vi + rv->vi);
    }

    else if (lhs->type.kind == TypeKind::Vector) {

      // vector + vector
      if (rhs->type.kind == TypeKind::Vector)
        lhs->As<ObjIterable>()->AppendList(PtrCast<ObjIterable>(rhs));

      // vector + any
      else
        lhs->As<ObjIterable>()->Append(rhs);
    }

    break;
  }

  case Kind::Sub: {

    if (lhs->is_numeric() && rhs->is_numeric()) {
      adjust_numeric_type_object(PtrCast<ObjPrimitive>(lhs),
                                 PtrCast<ObjPrimitive>(rhs));

      auto [lv, rv] = to_primitive_pair(lhs, rhs);

      if (lv->is_float())
        return ObjNew<ObjPrimitive>(lv->vf - rv->vf);

      if (lv->is_size())
        return ObjNew<ObjPrimitive>(lv->vsize - rv->vsize);

      return ObjNew<ObjPrimitive>(lv->vi - rv->vi);
    }

    break;
  }

  case Kind::Mul: {

    if (lhs->is_numeric() && rhs->is_numeric()) {
      auto [lv, rv] = to_primitive_pair(lhs, rhs);

      adjust_numeric_type_object(lv, rv);

      if (lv->is_float())
        return ObjNew<ObjPrimitive>(lv->vf * rv->vf);

      if (lv->is_size())
        return ObjNew<ObjPrimitive>(lv->vsize * rv->vsize);

      return ObjNew<ObjPrimitive>(lv->vi * rv->vi);
    }

    break;
  }

  case Kind::Div: {

    if (lhs->is_numeric() && rhs->is_numeric()) {
      auto [lv, rv] = to_primitive_pair(lhs, rhs);

      adjust_numeric_type_object(lv, rv);

      if (lv->is_float())
        return ObjNew<ObjPrimitive>(lv->vf / rv->vf);

      if (lv->is_size())
        return ObjNew<ObjPrimitive>(lv->vsize / rv->vsize);

      return ObjNew<ObjPrimitive>(lv->vi / rv->vi);
    }

    break;
  }

  case Kind::LShift:
  case Kind::RShift: {
    if (!lhs->is_int_or_size())
      Error(ast->lhs->token, "expected int or size")();
    else if (!rhs->is_int_or_size())
      Error(ast->rhs->token, "expected int or size")();

    auto [lv, rv] = to_primitive_pair(lhs, rhs);

    adjust_numeric_type_object(lv, rv);

    if (lv->is_size())
      return ObjNew<ObjPrimitive>(ast->kind == Kind::LShift
                                      ? lv->vsize << rv->vsize
                                      : lv->vsize >> rv->vsize);

    return ObjNew<ObjPrimitive>(ast->kind == Kind::LShift
                                    ? lv->vi << rv->vi
                                    : lv->vi >> rv->vi);
  }
  }

  Error(ast->token, "invalid operator")();
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
      fn_lvar.map[(*fn_arg_it++)->GetName()] = this->evaluate(arg);
    }

    this->evaluate(func->block);

    auto ret = fn_lvar.result;
    this->pop();

    return ret;
  }

  case Kind::Block: {
    auto _block = ASTCast<AST::Block>(ast);

    for (auto&& x : _block->list)
      this->evaluate(x);

    break;
  }

  case Kind::Function:
  case Kind::Enum:
  case Kind::Class:
    break;
  }

  if (ast->is_expr)
    return this->eval_expr(ASTCast<AST::Expr>(ast));

  return ObjNew<ObjNone>();
}

void Evaluator::do_eval() {
  stack.emplace_back(); // global variable

  for (auto&& x : this->root.list) {
    evaluate(x);
  }

  stack.clear();
}

} // namespace metro::eval