#include <cassert>

#include "alert.h"
#include "Utils.h"

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
                             scope ? scope : this->get_cur_scope(), false, true);

  if (!res.is_found()) {
    res.err_id = sr->nd_scope_resol_first;
    return res;
  }

  for (auto&& id : sr->nd_scope_resol_idlist) {
    string const& name = id->nd_id_name->str;

    switch (res.type) {
      case NameFindResult::NA_NotFound:
        res.err_id = id;
        return res;

      case NameFindResult::NA_Var:
      case NameFindResult::NA_Func:
      case NameFindResult::NA_Enumerator:
        Error(id->tok, "invalid use of scope resolution operator").crash();

      case NameFindResult::NA_Enum: {

        for (size_t i = 0; i < res.nd_enum->nd_enum_enumerators.size(); ++i) {
          auto e = res.nd_enum->nd_enum_enumerators[i];

          if (e->nd_enumerator_name->str == name) {
            res.type = NameFindResult::NA_Enumerator;
            res.nd_enumerator = e;
            res.enumerator_index = i;
            goto _sr_found;
          }
        }

        break;
      }

      case NameFindResult::NA_Class:
      case NameFindResult::NA_Struct:
      case NameFindResult::NA_Namespace:
        break;
    }

    res = this->find_name(id, res.scope, false, false);

  _sr_found:;
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

  auto const& name = id->nd_id_name->str;

  NameFindResult result{id->nd_id_name->str};

  for (auto&& targ : id->nd_id_template_args) {
    result.template_args.emplace_back(this->eval_expr_ti(targ));
  }

  auto& template_args = result.template_args;

  if (auto k = TypeInfo::get_kind_of_name(name); k != TypeKind::Unknown) {
    switch (k) {
      using K = TypeKind;

      case K::Int:
      case K::Float:
      case K::Bool:
      case K::Char:
      case K::String:
        if (template_args.size() >= 1)
          Error(id, "primitive type '" + name + "' is not template").crash();
        break;

      case K::Vector:
        if (template_args.size() != 1)
        _invalid_targ_label:
          Error(id, "invalid template arguments for built-in type '" + name + "'")
              .crash();
        break;

      case K::Tuple:
      case K::Functor:
        if (template_args.size() == 0)
          Error(id, "cannot use '" + name + "' without template arguments").crash();
        break;

      case K::Dict:
        if (template_args.size() != 2)
          goto _invalid_targ_label;
        break;

      case K::Type:
        todo_impl;

      default:
        panic;
    }

    result.type = NameFindResult::NA_PrimitiveType;
    result.primitive = TypeInfo(k, template_args);

    return result;
  }

  this->find_scope_if(
      [&](Scope* scope) -> bool {
        // variable
        if ((result.var = scope->find_var(id->tok->str))) {
          result.type = NameFindResult::NA_Var;
          result.scope = scope;
          return true;
        }

        // function
        if (scope->find_func(this, &template_args, result.fn_candidates, id->tok->str) >=
            1) {
          result.type = NameFindResult::NA_Func;
          result.scope = scope;

          if (auto E = this->ctx.evalctx; E && E->fn_candidates_out) {
            for (auto&& cd : result.fn_candidates)
              E->fn_candidates_out->push_back(cd->node);
          }

          return true;
        }

        // enum
        if (auto en = scope->find_if([&](Scope* S) -> bool {
              return S->type == SC_Enum && S->node->nd_enum_name->str == id->tok->str;
            })) {
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

TypeInfo Sema::make_functor_ti(Vec<TypeInfo> const& arg_types, TypeInfo const& ret_type) {
  TypeInfo ti(TypeKind::Functor);

  // { ret_type, arg_types... }
  ti.template_args = arg_types;
  ti.template_args.insert(ti.template_args.begin(), ret_type);

  return ti;
}

string Sema::get_full_name(Node* id_or_sr) {
  debug(assert(id_or_sr->is_id_or_sr()));

  if (id_or_sr->is(ND_ScopeResol)) {
    return get_full_name(id_or_sr->nd_scope_resol_first) +
           "::" + utils::join("::", id_or_sr->nd_scope_resol_idlist, get_full_name);
  }

  auto s = id_or_sr->get_name();

  if (!id_or_sr->nd_id_template_args.empty())
    s += "<" + utils::join(", ", id_or_sr->nd_id_template_args, get_full_name) + ">";

  return s;
}

Sema::TypeListCompareResult Sema::compare_type_list(Vec<TypeInfo> const& A,
                                                    Vec<TypeInfo> const& B) {
  TypeListCompareResult res = TLC_None;

  if (A.size() > B.size())
    res = static_cast<TypeListCompareResult>(res | TLC_Many);
  else if (A.size() < B.size())
    res = static_cast<TypeListCompareResult>(res | TLC_Few);
  else
    res = static_cast<TypeListCompareResult>(res | TLC_SameCount);

  for (size_t i = 0; i < std::min(A.size(), B.size()); i++) {
    if (!A[i].equals(B[i]))
      return static_cast<TypeListCompareResult>(res | TLC_Mismatch);
  }

  return static_cast<TypeListCompareResult>(res | TLC_Matched);
}

size_t Sema::find_function(Vec<Scope*>& out, Scope* in, string const& name,
                           Vec<TypeInfo> const& args, TypeInfo const& ret_type,
                           Scope* ignore_func) {
  for (auto&& func : in->functions) {

    auto fn = func->node;

    alert;
    if (func == ignore_func)
      continue;

    alert;
    if (!func->checked || func->get_name() != name)
      continue;

    alert;
    if (ignore_func->node->nd_func_is_variable_args != fn->nd_func_is_variable_args)
      continue;

    alert;
    if (!((TypeInfo*)ignore_func->node->nd_func_result_ti)->equals(func->ti))
      continue;

    auto res = this->compare_type_list(func->arg_types, args);

    if (res & TLC_PerfectMatch)
      out.push_back(func);
  }

  return out.size();
}
