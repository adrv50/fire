#include "Object.h"
#include "Error.h"
#include "Builtins.h"
#include "Sema/Sema.h"

namespace fire::sema {

ScopeContext* Sema::enter_scope(ScopeContext* scope) {
  assert(this->cur_scope->contains(scope));

  return this->cur_scope = scope;
}

void Sema::leave_scope() {
  this->cur_scope = this->cur_scope->parent;
}

Sema::Sema(Node* program)
    : program(program),
      root_scope(nullptr),
      cur_scope(nullptr),
      expr_eval(*this) {

  this->root_scope = ScopeContext::from_block(*this, program);

  this->cur_scope = this->root_scope;
}

void Sema::check_all() {
  for (auto&& nd : this->program->nd_block_items) {
    this->check_top_item(nd);
  }

  for (auto&& inst : this->tp_manager.instantiated_templates) {
    this->cur_scope = inst->sym->scope->parent;

    inst->sym->scope->node = inst->node;

    this->check_top_item(inst->node);
  }
}

void Sema::check_top_item(Node* node) {

  switch (node->kind) {
    case ND_Let:
      this->check_stmt(node);
      break;

    case ND_Function:
      if (!node->nd_func_is_template)
        this->check_func(node);

      break;

    case ND_Class:
      this->check_class(node);
      break;

    case ND_Namespace: {
      this->enter_scope(node->sema_ctx->scope);

      for (auto&& nd : node->list) {
        this->check_top_item(nd);
      }

      this->leave_scope();

      break;
    }
  }
}

void Sema::check_class(Node* node) {

  this->enter_scope(node->sema_ctx->scope);

  for (auto&& member : node->nd_class_fields->list) {
    this->check_let(member, node);
  }

  for (auto&& method : node->nd_class_methods->list) {
    this->check_func(method, node);
  }

  this->leave_scope();
}

void Sema::check_func(Node* node, Node* parent_class) {

  if (node->nd_func_is_template) {
    return;
  }

  auto fn_scope = this->enter_scope(node->sema_ctx->scope);

  for (auto v = fn_scope->varlist.begin(); auto&& arg : node->nd_func_args) {
    (*v)->type = this->eval_type_ti(arg->nd_func_arg_type);
    (*v)->is_type_deducted = true;
    v++;
  }

  this->check_stmt(node->nd_func_body);

  this->leave_scope();
}

void Sema::check_let(Node* node, Node* parent_class) {
  auto sym = node->sema_ctx->let_sym_ptr;

  bool type_spec = node->nd_let_type != nullptr;

  auto& type = sym->var->type;

  if (type_spec) {
    type = this->eval_type_ti(node->nd_let_type);
    sym->var->is_type_deducted = true;
  }

  if (node->nd_let_init) {
    if (!type_spec) {
      type = this->expr_eval(node->nd_let_init);
      sym->var->is_type_deducted = true;
    }
    else {
      this->expr_eval.ctx = {.allowed_empty_array = true,
                             .array_type_decl = node->nd_let_type,
                             .evaluated_array_type = &type};

      this->expr_eval.expect(node->nd_let_init, type);

      this->expr_eval.reset();
    }
  }
}

void Sema::check_stmt(Node* node) {
  switch (node->kind) {
    case ND_Block: {
      this->enter_scope(node->sema_ctx->scope);

      for (auto&& item : node->nd_block_items)
        this->check_stmt(item);

      this->leave_scope();

      break;
    }

    case ND_Let: {
      this->check_let(node);
      break;
    }

    case ND_If: {
      todo_impl;
    }

    case ND_Switch: {
      todo_impl;
    }

    case ND_Match: {
      todo_impl;
    }

    case ND_Return: {

      auto fnscope = this->get_cur_func_scope();

      if (auto x = node->nd_return_expr) {
        if (auto y = fnscope->node->nd_func_result_type)
          expr_eval.expect(x, this->eval_type_ti(fnscope->node->nd_func_result_type));
        else
          Error(x, "cannot use return value in this function")
              .add_note(Error(fnscope->node->nd_func_body->first_tok,
                              "insert type name before this token", ErrorType::Note)
                            .add_cursor_text("-> " + expr_eval(x).to_string()))
              .crash();
      }

      fnscope->func_ctx->return_stmt_list.emplace_back(node);

      break;
    }

    case ND_Break:
    case ND_Continue:
      todo_impl;

    default:
      this->expr_eval(node);
      break;
  }
}

TypeInfo Sema::eval_type_ti(Node* node) {
  if (!node)
    return TypeKind::None;

  Node* type_nd = node;

  Vec<Node*> sr = node->nd_type_scope_resol;

  sr.insert(sr.begin(), node);

  Vec<Symbol*> candidates;

  ScopeContext* scope = nullptr;

  Symbol* sym = nullptr;

  string name;

  Vec<TypeInfo> tp_args;

  for (auto&& nd : sr) {
    node = nd;
    name += nd->nd_type_id->nd_id_name->str;

    size_t count = this->find_name(candidates, nd->nd_type_id->nd_id_name->str, scope);

    if (count == 0) {
      Error(node, "cannot find type name '" + name + "'").crash();
    }
    else if (count >= 2) {
      todo_impl;
    }

    sym = candidates[0];

    bool have_tp_args = nd->nd_type_tp_args != nullptr;

    switch (sym->kind) {
      case SY_Namespace:
        if (have_tp_args)
          Error(nd, "namespace is not template").crash();

        if (nd == sr.back())
          Error(nd, "cannot use name of namespace as type name").crash();

        break;

      case SY_Enum:
        if (sym->decl->nd_enum_is_template) {
          if (!have_tp_args)
            goto _no_tp_args_err;
        }
        else if (have_tp_args)
          goto _not_template_err;

        break;

      case SY_Class:
        if (sym->decl->nd_class_is_template) {
          if (!have_tp_args)
            goto _no_tp_args_err;
        }
        else if (have_tp_args)
          goto _not_template_err;

        break;

      case SY_BuiltinType:
        // if vector or tuple or ...
        if (TypeInfo::is_template_kind(sym->tk)) {
          if (!have_tp_args)
            goto _no_tp_args_err;
        }
        else if (have_tp_args)
          goto _not_template_err;

        break;

      default:
        Error(nd->last_tok->next, "'" + name + "' is not a type name").crash();
    }

    if (nd->nd_type_tp_args) {
      tp_args.clear();

      for (auto&& tp : nd->nd_type_tp_args->list) {
        tp_args.emplace_back(this->eval_type_ti(tp));
      }
    }

    candidates.clear();
    scope = sym->scope;
    name += "::";

    continue;

  _not_template_err:
    Error(nd, "'" + name + "' is not template").crash();

  _no_tp_args_err:
    Error(nd, "cannot use '" + name + "' without template arguments").crash();
  }

  TypeInfo type;

  type.tp_args = tp_args;

  size_t tpcount = tp_args.size();

  switch (sym->kind) {
    case SY_Enum:
      type.kind = TypeKind::Enumerator;
      type.nd_enum = sym->decl;

      if (sym->decl->nd_enum_is_template) {
        if (tpcount == 0) {
          Error(node, "cannot use '" + name + "' without template arguments").crash();
        }
      }
      else if (tpcount >= 1) {
        Error(node->first_tok, "'" + name + "' is not template").crash();
      }

      break;

    case SY_Class:
      type.kind = TypeKind::Instance;
      type.nd_class = sym->decl;
      break;

    case SY_BuiltinType:
      type.kind = sym->tk;
      break;

    default:
      todo_impl;
  }

  type.is_mutable = node->nd_type_is_mut;
  type.is_reference = node->nd_type_is_ref;

  return type;
}

size_t Sema::find_name(Vec<Symbol*>& out, string const& name, ScopeContext* start) {
  size_t result = 0;

  if (!start)
    start = this->cur_scope;

  do {
    result = start->sym_table.find(out, name);
    start = start->parent;
  } while (start && result == 0);

  if (result == 0) {
    result = Builtins::Symbols::find(out, name);
  }

  return result;
}

} // namespace fire::sema