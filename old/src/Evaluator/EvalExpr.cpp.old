#include "Builtin.h"
#include "Evaluator.h"
#include "Error.h"

namespace fire::eval {

static inline ObjPtr<ObjPrimitive> new_int(i64 v) {
  return ObjNew<ObjPrimitive>(v);
}

static inline ObjPtr<ObjPrimitive> new_float(double v) {
  return ObjNew<ObjPrimitive>(v);
}

static inline ObjPtr<ObjPrimitive> new_bool(bool b) {
  return ObjNew<ObjPrimitive>(b);
}

static inline ObjPtr<ObjIterable> multiply_array(ObjPtr<ObjIterable> s, i64 n) {
  ObjPtr<ObjIterable> ret = PtrCast<ObjIterable>(s->Clone());

  while (--n) {
    ret->AppendList(s);
  }

  return ret;
}

static inline ObjPtr<ObjIterable> add_vec_wrap(ObjPtr<ObjIterable> v, ObjPointer e) {
  v = PtrCast<ObjIterable>(v->Clone());

  v->Append(e);

  return v;
}

ObjPointer Evaluator::eval_expr(ASTPtr<AST::Expr> ast) {
  using Kind = ASTKind;

  ObjPointer lhs = this->evaluate(ast->lhs);
  ObjPointer rhs = this->evaluate(ast->rhs);

  switch (ast->kind) {

  case Kind::Add: {

    if (lhs->is_vector() && rhs->is_int())
      return add_vec_wrap(PtrCast<ObjIterable>(lhs), rhs);

    if (rhs->is_vector() && lhs->is_int())
      return add_vec_wrap(PtrCast<ObjIterable>(rhs), lhs);

    switch (lhs->type.kind) {
    case TypeKind::Int:
      return new_int(lhs->get_vi() + rhs->get_vi());

    case TypeKind::Float:
      return new_float(lhs->get_vf() + rhs->get_vf());

    case TypeKind::String:
      lhs = lhs->Clone();
      lhs->As<ObjString>()->AppendList(PtrCast<ObjIterable>(rhs));
      return lhs;

    default:
      todo_impl;
    }

    break;
  }

  case Kind::Sub: {
    switch (lhs->type.kind) {
    case TypeKind::Int:
      return new_int(lhs->get_vi() - rhs->get_vi());

    case TypeKind::Float:
      return new_float(lhs->get_vf() - rhs->get_vf());
    }

    break;
  }

  case Kind::Mul: {
    if ((lhs->is_string() || lhs->is_vector()) && rhs->is_int())
      return multiply_array(PtrCast<ObjIterable>(lhs), rhs->get_vi());

    if ((rhs->is_string() || rhs->is_vector()) && lhs->is_int())
      return multiply_array(PtrCast<ObjIterable>(rhs), lhs->get_vi());

    switch (lhs->type.kind) {
    case TypeKind::Int:
      return new_int(lhs->get_vi() * rhs->get_vi());

    case TypeKind::Float:
      return new_float(lhs->get_vf() * rhs->get_vf());
    }

    break;
  }

  case Kind::Div: {
    switch (lhs->type.kind) {
    case TypeKind::Int: {
      auto vi = rhs->get_vi();

      if (vi == 0)
        goto _divided_by_zero;

      return new_int(lhs->get_vi() / vi);
    }

    case TypeKind::Float: {
      auto vf = rhs->get_vf();

      if (vf == 0)
        goto _divided_by_zero;

      return new_float(lhs->get_vf() / vf);
    }
    }

    break;
  }

  case Kind::Mod: {
    auto vi = rhs->get_vi();

    if (vi == 0)
      goto _divided_by_zero;

    return new_int(lhs->get_vi() % vi);
  }

  case Kind::LShift:
    return new_int(lhs->get_vi() << rhs->get_vi());

  case Kind::RShift:
    return new_int(lhs->get_vi() >> rhs->get_vi());

  case Kind::Bigger: {
    switch (lhs->type.kind) {
    case TypeKind::Int:
      return new_bool(lhs->get_vi() > rhs->get_vi());

    case TypeKind::Float:
      return new_bool(lhs->get_vf() > rhs->get_vf());

    case TypeKind::Char:
      return new_bool(lhs->get_vc() > rhs->get_vc());
    }

    break;
  }

  case Kind::BiggerOrEqual: {
    switch (lhs->type.kind) {
    case TypeKind::Int:
      return new_bool(lhs->get_vi() >= rhs->get_vi());

    case TypeKind::Float:
      return new_bool(lhs->get_vf() >= rhs->get_vf());

    case TypeKind::Char:
      return new_bool(lhs->get_vc() >= rhs->get_vc());
    }

    break;
  }

  case Kind::Equal: {
    return new_bool(lhs->Equals(rhs));
  }

  case Kind::LogAND:

  default:
    not_implemented("not implemented operator: " << lhs->type.to_string() << " "
                                                 << ast->op.str << " "
                                                 << rhs->type.to_string());
  }

  return lhs;

_divided_by_zero:
  throw Error(ast->op, "divided by zero");
}

} // namespace fire::eval