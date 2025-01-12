#include "Object.h"
#include "Sema/Sema.h"

namespace fire::sema {

ExprEval::ExprEval(Sema& S)
    : S(S) {
}

void ExprEval::save() {
  this->_saves.push_back(this->ctx);
}

void ExprEval::restore() {
  this->ctx = this->_saves.back();
  this->_saves.pop_back();
}

TypeInfo ExprEval::eval(Node* node) {
  switch (node->kind) {
    case ND_Value:
      return node->nd_value->ti;

    case ND_Identifier: {

      todo_impl;
    }
  }

  assert(node->kind >= ND_Mul && node->kind <= ND_Assign);

  auto lhs = this->eval(node->nd_lhs);
  auto rhs = this->eval(node->nd_rhs);

  if (!lhs.equals(rhs)) {
    Error(node->tok, "cannot use operator for not same type").crash();
  }

  switch (node->kind) {
    case ND_Add:
      break;
  }

  return lhs;
}

TypeInfo ExprEval::make_type_from_symbol(Symbol* sym) {
  switch (sym->kind) {
    case SY_Var:
      return sym->var->type;

    case SY_Func: {
      todo_impl;
    }
  }

  todo_impl;
}

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
    if (nd->is(ND_Function))
      this->check_func(nd);
    else if (nd->is(ND_Let))
      this->check_stmt(nd);
  }
}

void Sema::check_func(Node* node) {

  auto fn_scope = this->enter_scope(node->sema_ctx->scope);

  for (auto v = fn_scope->varlist.begin(); auto&& arg : node->nd_func_args) {
    (*v)->type = this->eval_type_ti(arg->nd_func_arg_type);
    (*v)->is_type_deducted = true;
    v++;
  }

  this->check_stmt(node->nd_func_body);

  this->leave_scope();
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

    default:
      this->expr_eval(node);
      break;
  }
}

TypeInfo Sema::eval_type_ti(Node* node) {

  return {};
}

size_t Sema::find_name(Vec<Symbol*> out, string const& name, ScopeContext* start) {
  size_t result = 0;

  do {
    result = start->sym_table.find(out, name);
    start = start->parent;
  } while (result == 0);

  return result;
}

} // namespace fire::sema