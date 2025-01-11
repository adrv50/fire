#include "Sema_2.h"

namespace sema {

Sema::Sema(Node* program)
    : Program(program),
      RootScope(ScopeContext::from_block(program)),
      CurScope(RootScope) {
}

void Sema::check_full() {

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
      this->expect_type({.in_statement = true, .stmt_nd = node}, node->nd_if_cond,
                        TypeKind::Bool);

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
      this->expect_type({.in_statement = true, .stmt_nd = node}, node->nd_while_cond,
                        TypeKind::Bool);

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
        auto type = this->eval_expr_ti(x, {.in_statement = true, .stmt_nd = node});

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
    auto context =
        ExprEvalContext{.container_elem_type_p = node->nd_let_type ? &var.type : nullptr,
                        .container_elem_type_def_nd = node->nd_let_type,
                        .in_statement = true,
                        .stmt_nd = node};

    if (node->nd_let_type)
      this->expect_type(context, node->nd_let_init, var.type);
    else
      this->eval_expr_ti(node->nd_let_init, context);

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

    case ND_CallConstructor: {
      auto ctor_id = this->eval_id(ctx, node->nd_callctor_ctor_side);

      switch (ctor_id.info.kind) {
        case ID_Enumerator: {
          todo_impl;
        }

        case ID_Class: {
          todo_impl;
        }
      }

      Error(node->nd_callctor_ctor_side,
            "'" + ctor_id.info.name + "' is not enumerator or class")
          .crash();
    }

    case ND_Array: {
      if (node->nd_elements.empty()) {
        if (!ctx.container_elem_type_p)
          Error(node, "cannot deduct type of empty array").crash();

        if (!ctx.container_elem_type_p->is(TypeKind::Vector)) {
          auto s = ctx.container_elem_type_p->to_string();

          Error(ctx.container_elem_type_def_nd,
                "expected 'vector<...>' because array expression are used in "
                "initializer, but found '" +
                    s + "'")
              .add_cursor_text("did you mean 'vector<" + s + ">' ?")
              .crash();
        }

        alertmsg(ctx.container_elem_type_p->to_string());

        return *ctx.container_elem_type_p;
      }

      auto it = node->nd_elements.begin();
      auto type = this->eval_expr_ti(*it, ctx);

      for (++it; it != node->nd_elements.end(); it++)
        this->expect_type(ctx, *it, type);

      return TypeInfo(TypeKind::Vector, {type});
    }

    case ND_Tuple: {
      TypeInfo type = TypeKind::Tuple;

      for (auto&& elem : node->nd_elements)
        type.append_template_arg(this->eval_expr_ti(elem, ctx));

      return type;
    }

    case ND_Dict: {
      auto it = node->nd_dict_pairs.begin();

      auto key = this->eval_expr_ti((*it)->nd_dict_pair_key, ctx);
      auto val = this->eval_expr_ti((*it)->nd_dict_pair_value, ctx);

      for (++it; it != node->nd_dict_pairs.end(); it++) {
        this->expect_type(ctx, (*it)->nd_dict_pair_key, key);
        this->expect_type(ctx, (*it)->nd_dict_pair_value, val);
      }

      return TypeInfo(TypeKind::Dict, {key, val});
    }

    case ND_Not:
      return this->expect_type(ctx, node->nd_lhs, TypeKind::Bool);

    case ND_Ref:
      todo_impl;

    case ND_Cast:
      todo_impl;

    case ND_Subscript: {
      auto arr = this->eval_expr_ti(node->nd_lhs, ctx);
      auto index = this->eval_expr_ti(node->nd_rhs, ctx);

      if (!arr.is(TypeKind::Vector))
        Error(node->tok, "'" + arr.to_string() + "' type object is not subscriptable")
            .crash();

      if (!index.is(TypeKind::Int)) {
        Error(node->nd_rhs, "indexer must be integer.").crash();
      }

      return arr.tp_args[0];
    }

    case ND_MemberAccess: {
      auto objtype = this->eval_expr_ti(node->nd_lhs, ctx);

      if (objtype.is(TypeKind::Instance)) {
      }
    }

    case ND_CallFunc: {
      auto chk_result = this->check_call_func_expr(ctx, node);

      return chk_result.result_type;
    }

    default:
      break;
  }

  auto lhs = this->eval_expr_ti(node->nd_lhs, ctx);
  auto rhs = this->eval_expr_ti(node->nd_rhs, ctx);

  if (!lhs.equals(rhs))
    Error(node->tok, "only can use expression operator for same type").crash();

  /** Todo!! **/

  return lhs;
}

TypeInfo Sema::eval_type_ti(Node* node) {

  if (auto k = TypeInfo::get_kind_of_name(node->nd_type_name->str);
      k != TypeKind::Unknown) {

    TypeInfo type = k;

    for (auto&& t_arg : node->nd_type_tp_args) {
      type.tp_args.emplace_back(this->eval_type_ti(t_arg));
    }

    return type;
  }

  Error(node, "unknown type name").crash();
}

} // namespace sema