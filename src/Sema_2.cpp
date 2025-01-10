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
  auto funcScope = node->sema_scope;

  this->enter_scope(funcScope);

  auto func_ctx = funcScope->func_ctx;

  for (size_t i = 0; i < node->nd_func_args.size(); i++) {
    auto iter = node->nd_func_args.begin() + i;
    auto& arg_vi = funcScope->variables[i];

    arg_vi.type = this->eval_type_ti((*iter)->nd_func_arg_type);

    if (auto def = std::find_if(node->nd_func_args.begin(), iter,
                                [&](Node* nd) {
                                  return nd->nd_func_arg_name->str !=
                                         (*iter)->nd_func_arg_name->str;
                                });
        def != iter) {
      Error(*iter, "duplicate argument name '" + (*iter)->nd_func_arg_name->str + "'")
          .add_note(*def, "first defined here")
          .crash();
    }
  }

  if (node->nd_func_result_type)
    func_ctx->result_type = this->eval_type_ti(node->nd_func_result_type);

  this->check_block(node->nd_func_body);

  this->leave_scope();
}

void Sema::check_block(Node* node) {
  if (!node)
    return;

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

    case ND_If: {
      this->expect_type({}, node->nd_if_cond, TypeKind::Bool);

      this->check_block(node->nd_if_then);
      this->check_block(node->nd_if_else);

      break;
    }

    case ND_Switch: {
      todo_impl;
    }

    case ND_Match: {
      todo_impl;
    }

    case ND_Loop:
      this->check_block(node->nd_loop_body);
      break;

    case ND_While:
      this->expect_type({}, node->nd_while_cond, TypeKind::Bool);
      this->check_block(node->nd_while_body);
      break;

    case ND_For: {
      todo_impl;
    }

    case ND_ForEach: {
      todo_impl;
    }

    case ND_ForRange: {
      todo_impl;
    }

    case ND_Return: {
      if (auto x = node->nd_return_expr) {
        auto type = this->eval_expr_ti(x, {});

        if (auto ctx = this->get_cur_func_scope()->func_ctx;
            !ctx->result_type.equals(type)) {
          Error(x, "mismatched function result type")
              .add_note(ctx->result_type_nd, "defined here")
              .crash();
        }
      }

      break;
    }

    case ND_Break: {
      todo_impl;
    }

    case ND_Continue: {
      todo_impl;
    }

    default:
      this->eval_expr_ti(node, {});
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
        x && !var.type.equals(this->eval_expr_ti(node->nd_let_init, {}))) {
      Error(node->nd_let_init, "type mismatch").emit();
    }

    var.type = this->eval_expr_ti(node->nd_let_init, {});
    var.is_type_deducted = true;
  }

  if (auto cur_func = this->get_cur_func_scope()) {
    var.offset = cur_func->func_ctx->pvar_list.size();

    cur_func->func_ctx->pvar_list.push_back(&var);
  }
}

TypeInfo Sema::eval_expr_ti(Node* node, ExprEvalContext ctx) {

  if (!node)
    return TypeKind::None;

  switch (node->kind) {
    case ND_Value:
      return node->obj->ti;

    case ND_Identifier:
    case ND_ScopeResol:
      return this->eval_id(ctx, node).type;

    case ND_CallFunc: {
      todo_impl;
    }

    default:
      break;
  }

  auto lhs = this->eval_expr_ti(node->nd_lhs, ctx);
  auto rhs = this->eval_expr_ti(node->nd_rhs, ctx);

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