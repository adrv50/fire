#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace metro::eval {

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
  }

  Error(ast->token, "invalid operator")();
}

} // namespace metro::eval