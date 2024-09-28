#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace fire::eval {

using namespace AST;

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

  case Kind::Equal: {
    return ObjNew<ObjPrimitive>((bool)lhs->Equals(rhs));
  }

  case Kind::Bigger:
  case Kind::BiggerOrEqual: {
    auto ret = ObjNew<ObjPrimitive>(false);
    bool or_equal = ast->kind == Kind::BiggerOrEqual;

    if (lhs->is_numeric(true) && rhs->is_numeric(true)) {
      auto [lv, rv] = to_primitive_pair(lhs, rhs);

      adjust_numeric_type_object(lv, rv);

      if (lv->is_int())
        ret->vb =
            lv->vi > rv->vi || (or_equal ? lv->vi == rv->vi : 0);
      else if (lv->is_size())
        ret->vb =
            lv->vs > rv->vs || (or_equal ? lv->vs == rv->vs : 0);
      else if (lv->is_float())
        ret->vb =
            lv->vf > rv->vf || (or_equal ? lv->vf == rv->vf : 0);
    }
    else {
      break;
    }

    return ret;
  }

  case Kind::BitAND:
  case Kind::BitXOR:
  case Kind::BitOR: {
    if (!lhs->is_integer() || !rhs->is_integer())
      break;

    auto [lv, rv] =
        adjust_numeric_type_object(to_primitive_pair(lhs, rhs));

    switch (ast->kind) {
    case Kind::BitAND:
      lv->is_int() ? (lv->vi &= rv->vi) : lv->vs &= rv->vs;
      break;

    case Kind::BitXOR:
      lv->is_int() ? (lv->vi ^= rv->vi) : lv->vs ^= rv->vs;
      break;

    case Kind::BitOR:
      lv->is_int() ? (lv->vi |= rv->vi) : lv->vs |= rv->vs;
      break;
    }

    return lv;
  }

  case Kind::LogAND:
  case Kind::LogOR: {
    if (!lhs->is_boolean() || !rhs->is_boolean())
      break;

    auto [a, b] = pair_ptr_cast<ObjPrimitive>(lhs, rhs);

    return ObjNew<ObjPrimitive>(ast->kind == Kind::LogAND
                                    ? (a->vb && b->vb)
                                    : a->vb || b->vb);
  }
  }

  Error(ast->token, "invalid operator '" + ast->op.str + "' with (" +
                        lhs->type.to_string() + ", " +
                        rhs->type.to_string() + ")")();
}

} // namespace fire::eval