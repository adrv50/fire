#include "Object.h"
#include "Driver/Error.h"
#include "Builtins.h"
#include "Sema/Sema.h"

namespace fire::sema {

Sema::Sema(Node* program)
    : program(program),
      root_scope(nullptr),
      cur_scope(nullptr),
      expr_eval(*this) {

  this->root_scope = ScopeContext::from_block(*this, program);

  this->cur_scope = this->root_scope;
}

ScopeContext* Sema::enter_scope(ScopeContext* scope) {
  assert(this->cur_scope->contains(scope));

  return this->cur_scope = scope;
}

void Sema::leave_scope() {
  this->cur_scope = this->cur_scope->parent;
}

void Sema::check_all() {
  for (auto&& nd : this->program->nd_block_items) {
    this->check_top_item(nd);
  }

  for (auto&& inst : this->tp_manager.instantiated_templates) {

    auto func_scope = inst->sym->scope;

    func_scope->node = inst->node;

    this->cur_scope = func_scope->parent;

    this->check_top_item(inst->node);
  }
}

void Sema::check_top_item(Node* node) {

  switch (node->kind) {
    case ND_Let:
      this->check_stmt(node);

      node->sema_ctx->let_sym_ptr->var->offset_in_stack =
          this->global_or_namespace_var_offset++;

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
  if (!node)
    return;

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
      this->expr_eval.expect(node->nd_if_cond, TypeKind::Bool);
      this->check_stmt(node->nd_if_then);
      this->check_stmt(node->nd_if_else);
      break;
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
              .add_note(Error(fnscope->node->nd_func_body->first_tok->prev,
                              "insert type name after this token", ErrorType::Note)
                            .add_errpos_insert_text(" -> " + expr_eval(x).to_string(), 1))
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

  // 初期化
  Vec<Node*> sr{node}; // スコープ解決リストを最初からnodeで初期化
  sr.insert(sr.end(), node->nd_type_scope_resol.begin(),
            node->nd_type_scope_resol.end()); // スコープ解決リストを追加

  Vec<Symbol*> candidates;
  ScopeContext* scope = nullptr;
  Symbol* sym = nullptr;
  string name;

  Vec<TypeInfo> tp_args; // テンプレート引数のリスト

  // 名前解決処理
  for (auto&& nd : sr) {
    node = nd;
    name += nd->nd_type_id->nd_id_name->str;

    size_t count = this->find_name(candidates, nd->nd_type_id->nd_id_name->str, scope);

    // 名前が見つからない、または候補が複数ある場合のエラーチェック
    if (count == 0) {
      Error(node, "cannot find type name '" + name + "'").crash();
    }
    if (count >= 2) {
      todo_impl; // 複数候補が見つかった場合の処理
    }

    sym = candidates[0]; // 最初の候補を採用

    // テンプレート引数の処理
    if (nd->nd_type_tp_args_ptr != nullptr) {
      tp_args.clear();
      for (auto&& type : nd->nd_type_tp_args) {
        tp_args.emplace_back(this->eval_type_ti(type));
      }
    }

    // 名前空間、列挙型、クラス型、ビルトイン型のチェック
    this->handle_type_kind_error(sym, nd, tp_args, name);

    candidates.resize(0);
    scope = sym->get_parent_scope();
    name += "::"; // 名前の後ろに'::'を追加
  }

  // TypeInfoの設定
  TypeInfo type;
  type.tp_args = tp_args; // テンプレート引数を設定
  size_t tpcount = tp_args.size();

  // 型ごとの処理
  switch (sym->kind) {
    case SY_Enum:
      type.kind = TypeKind::Enumerator;
      type.nd_enum = sym->decl;

      // 列挙型がテンプレートの場合
      if (sym->decl->nd_enum_is_template && tpcount == 0) {
        Error(node, "cannot use '" + name + "' without template arguments").crash();
      }
      else if (!sym->decl->nd_enum_is_template && tpcount >= 1) {
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

void Sema::handle_type_kind_error(Symbol* sym, Node* nd, const Vec<TypeInfo>& tp_args,
                                  const string& name) {
  bool have_tp_args = !tp_args.empty();

  switch (sym->kind) {
    case SY_Namespace:
      if (have_tp_args) {
        Error(nd, "namespace is not template").crash();
      }
      if (nd == nd->nd_type_scope_resol.back()) {
        Error(nd, "cannot use name of namespace as type name").crash();
      }
      break;

    case SY_Enum:
      if (sym->decl->nd_enum_is_template && !have_tp_args) {
        Error(nd, "cannot use '" + name + "' without template arguments").crash();
      }
      else if (!sym->decl->nd_enum_is_template && have_tp_args) {
        Error(nd, "'" + name + "' is not template").crash();
      }
      break;

    case SY_Class:
      if (sym->decl->nd_class_is_template && !have_tp_args) {
        Error(nd, "cannot use '" + name + "' without template arguments").crash();
      }
      else if (!sym->decl->nd_class_is_template && have_tp_args) {
        Error(nd, "'" + name + "' is not template").crash();
      }
      break;

    case SY_BuiltinType:
      if (TypeInfo::is_template_kind(sym->tk)) {
        if (!have_tp_args) {
          Error(nd, "cannot use '" + name + "' without template arguments").crash();
        }
        size_t least = TypeInfo::get_least_template_args_count_of(sym->tk);
        if (tp_args.size() < least) {
          Error(nd, "too few template arguments").crash();
        }
      }
      else if (have_tp_args) {
        Error(nd, "'" + name + "' is not template").crash();
      }
      break;

    default:
      Error(nd->last_tok->next, "'" + name + "' is not a type name").crash();
  }
}
size_t Sema::find_name(Vec<Symbol*>& out, string const& name, ScopeContext* start,
                       bool once) {
  if (!start)
    start = this->cur_scope;

  do {
    start->sym_table.find(out, name);

    if (once)
      goto __last;

    start = start->parent;
  } while (start && out.empty());

  if (out.empty()) {
    Builtins::Symbols::find(out, name);
  }

__last:;
  return out.size();
}

} // namespace fire::sema