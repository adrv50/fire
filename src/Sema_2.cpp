#include "Sema_2.h"

namespace sema {

Sema::Sema(Node* program)
    : Program(program),
      RootScope(ScopeContext::from_block(program)),
      CurScope(RootScope) {
}

void Sema::check_full() {

  alert;

  for (auto&& item : this->Program->nd_items) {
    switch (item->kind) {
      case ND_Let:
        this->check_let(item);
        break;

      case ND_Function:
        this->check_func(item);
        break;
    }
  }
}

void Sema::check_func(Node* node) {
  this->enter_scope(node->sema_scope);

  this->check_block(node->nd_func_body);

  this->leave_scope();
}

void Sema::check_block(Node* node) {
  this->enter_scope(node->sema_scope);

  for (auto&& item : node->nd_items) {
    this->check_stmt(item);
  }

  this->leave_scope();
}

void Sema::check_stmt(Node* node) {
  switch (node->kind) {
    case ND_Block:
      this->check_block(node);
      break;

    case ND_Let:
      this->check_let(node);
      break;

    default:
      this->eval_expr_ti(node);
      break;
  }
}

void Sema::check_let(Node* node) {
  auto& var = get_varinfo_from_let_nd(node);

  if (node->nd_let_type) {
    var.type = this->eval_type_ti(node->nd_let_type);
    var.is_type_deducted = true;
  }

  if (node->nd_let_init) {
    if (auto x = node->nd_let_type;
        x && !var.type.equals(this->eval_expr_ti(node->nd_let_init))) {
      Error(node->nd_let_init, "type mismatch").emit();
    }

    var.type = this->eval_expr_ti(node->nd_let_init);
    var.is_type_deducted = true;
  }
}

TypeInfo Sema::eval_expr_ti(Node* node) {

  if (!node)
    return TypeKind::None;

  switch (node->kind) {
    case ND_Value:
      return node->obj->ti;

    case ND_Identifier:
    case ND_ScopeResol:
      return this->eval_id(node);

    case ND_CallFunc: {
      todo_impl;
    }

    default:
      break;
  }

  auto lhs = this->eval_expr_ti(node->nd_lhs);
  auto rhs = this->eval_expr_ti(node->nd_rhs);

  if (!lhs.equals(rhs))
    Error(node->tok, "only can use expression operator for same type").crash();

  return lhs;
}

TypeInfo Sema::eval_type_ti(Node* node) {

  if (auto k = TypeInfo::get_kind_of_name(node->nd_type_name->str);
      k != TypeKind::Unknown) {
    return k;
  }

  Error(node, "unknown type name").crash();
}

} // namespace sema