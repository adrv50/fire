#include <cassert>

#include "alert.h"

#include "Builtins.h"
#include "TypeInfo.h"
#include "Token.h"
#include "Node.h"

#include "Sema.h"
#include "Error.h"

static bool _is_in_func_keep = false;

// ---------------------------------
//  enter_func
// ---------------------------------
Sema::Scope* Sema::enter_func(Node* func) {
  _is_in_func_keep = this->ctx.is_in_func;

  this->ctx.is_in_func = true;
  return this->ctx.enter(func);
}

// ---------------------------------
//  leave_func
// ---------------------------------
void Sema::leave_func(Node* func) {
  assert(this->ctx.cur_scope->node == func);

  this->ctx.leave();
  this->ctx.is_in_func = _is_in_func_keep;
}

// ---------------------------------
//  err_if_unexpected_type
// ---------------------------------
void Sema::err_if_unexpected_type(TypeInfo const& expection, Node* to_expect) {
  if (!to_expect)
    return;

  if (auto ti = this->eval_expr_ti(to_expect); !expection.equals(ti))
    Error(to_expect,
          "expected '" + expection.to_string() + "', but found '" + ti.to_string() + "'")
        .crash();
}

//
// is_method:
//   check if the function is method.
//
bool Sema::is_method(Node* func) {
  return func->nd_func_is_method;
}

bool Sema::is_method(Builtins::BuiltinFunc const* bf) {
  return bf->is_method;
}

//
// get_cur_scope:
//   get current scope.
//
Sema::Scope*& Sema::get_cur_scope() {
  return this->ctx.cur_scope;
}

//
// find_scope:
//   find scope by predicate.
//
Sema::Scope* Sema::find_scope_if(std::function<bool(Scope*)> const& pred,
                                 Scope* from_this, bool from_root, bool reverse) {

  Scope* scope = nullptr;

  if (from_root)
    scope = this->root_scope;
  else if (!(scope = from_this))
    scope = this->get_cur_scope();

  while (!pred(scope)) {
    if (reverse) {
      if (scope->parent)
        scope = scope->parent;
      else
        return nullptr;
    }
    else {
      for (auto&& child : scope->childs)
        if (auto found = this->find_scope_if(pred, child, false, reverse))
          return found;

      return nullptr;
    }
  }

  return scope;
}

//
// scope_resolution:
//   wrapper for scope resolution operator.
//
Sema::NameFindResult Sema::scope_resolution(Node* sr, Scope* scope) {
  (void)scope;

  auto res = this->find_name(sr->nd_scope_resol_first,
                             scope ? scope : this->get_cur_scope(), false, false);

  if (!res.is_found()) {
    res.err_id = sr->nd_scope_resol_first;
    return res;
  }

  for (auto&& id : sr->nd_scope_resol_idlist) {
    switch (res.type) {
    case NameFindResult::NA_NotFound:
      res.err_id = id;
      return res;

    case NameFindResult::NA_Var:
    case NameFindResult::NA_Func:
      Error(id->tok, "cannot use scope resolution operator for variable or function")
          .crash();

    case NameFindResult::NA_Enum:
      todo_impl;
      break;

    case NameFindResult::NA_Class:
    case NameFindResult::NA_Struct:
    case NameFindResult::NA_Namespace:
      break;
    }

    res = this->find_name(id, res.scope, false, false);
  }

  return res;
}

//
// find_name:
//   find name in current scope.
//
Sema::NameFindResult Sema::find_name(Node* id, Scope* from_this, bool from_root,
                                     bool reverse) {
  Scope* scope = from_this;

  if (!scope)
    scope = from_root ? this->root_scope : this->get_cur_scope();

  NameFindResult result{id->nd_id_name->str};

  this->find_scope_if(
      [&](Scope* scope) -> bool {
        // variable
        if ((result.var = scope->find_var(id->tok->str))) {
          result.type = NameFindResult::NA_Var;
          result.scope = scope;
          return true;
        }

        // function
        if (auto fn = scope->find_func(id->tok->str)) {
          result.type = NameFindResult::NA_Func;
          result.scope = fn;
          result.func = fn->node;
          return true;
        }

        // enum
        if (auto en = scope->find_if([](Scope* S) -> bool {
              return S->type == SC_Enum;
            })) {
          alert;
          result.type = NameFindResult::NA_Enum;
          result.scope = en;
          result.nd_enum = en->node;
          return true;
        }

        return false;
      },
      nullptr, from_root, reverse);

  if (result.is_found())
    return result;

  if (reverse) {
    if (scope->parent)
      result = this->find_name(id, scope->parent, false, true);
  }
  else {
    for (auto&& child : scope->childs)
      if ((result = this->find_name(id, child, false, false)).is_found())
        break;
  }

  if (!result.is_found())
    result.err_id = id;

  return result;
}

Sema::NameFindResult Sema::find_name_wrap(Node* name, Scope* scope) {
  return name->is(ND_ScopeResol) ? this->scope_resolution(name, scope)
                                 : this->find_name(name, scope);
}
