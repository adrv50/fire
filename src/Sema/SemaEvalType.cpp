#include <cassert>

#include "alert.h"
#include "Utils.h"

#include "Token.h"
#include "Node.h"

#include "Sema.h"
#include "Error.h"

// ---------------------------------
//  limit_bf_candidates
//    limit builtin function candidates.
// ---------------------------------
auto limit_bf_candidates(Vec<Builtins::BuiltinFunc const*>& vec,
                         Vec<TypeInfo> const& args, bool is_method,
                         TypeInfo const* self) {

  auto pred = [&](Builtins::BuiltinFunc const* bf) {
    if (bf->is_method != is_method)
      return true;

    if (bf->is_method && (!self || !self->equals(bf->self_type)))
      return true;

    if (bf->arg_types.size() > args.size() ||
        (!bf->is_variable_args && bf->arg_types.size() < args.size()))
      return true;

    for (size_t i = 0; i < bf->arg_types.size(); i++)
      if (!bf->arg_types[i].equals(args[i]))
        return true;

    return false;
  };

  return std::remove_if(vec.begin(), vec.end(), pred);
}

// ---------------------------------
//  check_function_call
//    check function call.
// ---------------------------------
TypeInfo Sema::check_function_call(Node* call) {

  Vec<TypeInfo> call_args;

  for (auto&& arg : call->nd_callfunc_args)
    call_args.push_back(this->eval_expr_ti(arg));

  string strargs = "(" +
                   utils::join(", ", call_args,
                               [](TypeInfo const& t) -> string {
                                 return t.to_string();
                               }) +
                   ")";

  // auto res = this->find_name_wrap(call->nd_callfunc_callee);
  auto res =
      this->find_name(call->nd_callfunc_callee, this->get_cur_scope(), false, true);

  auto userdef = res.func;

  // if not found user-defined, find builtin function
  // ユーザー定義関数がない場合は組み込み関数を探す
  if (!userdef) {
    auto id = call->nd_callfunc_callee->get_last_id();

    auto name = id->tok->str;

    auto is_method_call = call->nd_callfunc_is_method_call;

    TypeInfo self_ti;

    if (is_method_call) {
      self_ti = this->eval_expr_ti(call->nd_callfunc_method_self);
    }

    // if found builtin function, set pointer
    // 組み込み関数が存在する
    if (Vec<Builtins::BuiltinFunc const*> bfs;
        Builtins::BuiltinFunc::find(bfs, name) != 0) {

      auto iter = limit_bf_candidates(bfs, call_args, is_method_call, &self_ti);

      if (auto count = std::distance(bfs.begin(), iter); count >= 2) {
        Error(id, "ambiguous call to builtin function '" + name + strargs + "'").crash();
      }
      else if (count == 0) {
        Error e{id};

        if (is_method_call)
          e.set_message("no overload found for builtin method '" + self_ti.to_string() +
                        "::" + name + strargs + "'");
        else
          e.set_message("no overload found for builtin function '" + name + strargs +
                        "'");

        while (iter != bfs.end())
          e.add_note("candidate: " + (*iter++)->to_string());

        e.crash();
      }

      auto& bf = bfs[0];

      // compare arguments
      // 引数を比較する
      this->compare_call_arguments(call, this->is_method(bf), call_args, bf->arg_types,
                                   bf->is_variable_args, nullptr, bf);

      // set pointer
      // ポインタを設定する
      call->nd_callfunc_callee_builtin = bf;

      // return type
      // 戻り値の型を返す
      return bf->ret_type;
    }

    // if not found builtin function, error
    // 組み込み関数が見つからなかった場合はエラー
    else {
      Error(id->tok, "cannot find the " + string(is_method_call ? "method" : "function") +
                         " '" + name + strargs + "'")
          .crash();
    }
  }

  // --------
  // exist user-defined function same name
  // 同じ名前のユーザー定義関数が存在する

  // compare arguments
  // 引数を比較する
  this->compare_call_arguments(call, this->is_method(userdef), call_args,
                               res.scope->arg_types, userdef->nd_func_is_variable_args,
                               userdef, nullptr);

  // set pointer
  // ポインタを設定する
  call->nd_callfunc_callee_userdef = userdef;

  // return type
  // 戻り値の型を返す
  return this->eval_type_ti(userdef->nd_func_result_type);
}

// ---------------------------------
//  compare_call_arguments:
//    compare function call arguments.
// ---------------------------------
void Sema::compare_call_arguments(Node* cf, bool is_method, Vec<TypeInfo> const& call,
                                  Vec<TypeInfo> const& func, bool is_variable_args,
                                  Node* fn, Builtins::BuiltinFunc const* bfn) {

  (void)is_method;

  if (!is_variable_args && func.size() < call.size()) {
    Error(cf->nd_callfunc_callee->get_last_id(),
          "too many arguments to call function '" +
              (fn ? fn->nd_func_name->str : bfn->name) + "'")
        .add_note(fn,
                  fn ? "defined here"
                     : ("builtin function '" + bfn->name + "' can take up to " +
                        std::to_string(bfn->arg_types.size()) + " arguments."),
                  ErrorType::Note)
        .crash();
  }

  if (call.size() < func.size()) {
    Error(cf->nd_callfunc_callee->get_last_id(),
          "too few arguments to call function '" +
              (fn ? fn->nd_func_name->str : bfn->name) + "'")
        .add_note(fn,
                  fn ? "defined here"
                     : ("least " + std::to_string(bfn->arg_types.size()) +
                        " arguments needed by builtin function '" + bfn->name + "'"),
                  ErrorType::Note)
        .crash();
  }

  for (size_t i = 0; i < func.size(); i++) {
    auto& callarg = call[i];
    auto& funcarg = func[i];

    if (!callarg.equals(funcarg)) {
      Error(cf->nd_callfunc_callee->get_last_id(), "expected '" + funcarg.to_string() +
                                                       "' type expression, but found '" +
                                                       callarg.to_string() + "'")
          .add_note(fn ? fn->nd_func_args[i]->nd_func_arg_type : nullptr,
                    fn ? "defined here" : "definition is: " + bfn->to_string())
          .crash();
    }
  }
}

// ---------------------------------
//  eval_expr_ti
//    evaluate expression type info.
// ---------------------------------
TypeInfo Sema::eval_expr_ti(Node* node) {

  switch (node->kind) {

  case ND_Value:
    return node->nd.obj->ti;

  case ND_Array: {
    auto const& elems = node->nd_array_elements;

    if (elems.empty()) {
      // todo: check type of empty array from near context.
      todo_impl;
    }

    TypeInfo ti = this->eval_expr_ti(elems[0]);

    for (size_t i = 1; i < elems.size(); i++)
      this->err_if_unexpected_type(ti, elems[i]);

    return ti;
  }

  case ND_Tuple: {
    auto const& elems = node->nd_tuple_elements;

#if _FIRE_DEBUG_
    if (elems.empty()) {
      // Why empty!?? may parser have bugs.
      panic;
    }
#endif

    TypeInfo ti{TypeKind::Tuple};

    for (auto&& elem : elems)
      ti.append_template_arg(this->eval_expr_ti(elem));

    return ti;
  }

  case ND_Dict: {
    return TypeInfo(TypeKind::Dict, {this->eval_expr_ti(node->nd_dict_pair_key),
                                     this->eval_expr_ti(node->nd_dict_pair_value)});
  }

  case ND_Identifier:
  case ND_ScopeResol: {

    auto res = node->is(ND_Identifier)
                   ? this->find_name(node, this->get_cur_scope(), false, true)
                   : this->scope_resolution(node);

    //
    // Variable
    if (res.var) {
      if (!res.var->is_type_deducted)
        Error(node, "cannot use variable before type deducted").crash();

      node->id_kind = NodeIdentifierKind::ID_Var;

      node->nd_variable_offset = res.var->index;
      node->nd_variable_is_global = (res.scope == this->root_scope);

      node->nd_id_target = res.var->decl;

      return res.var->ti;
    }

    //
    // function name
    //   => Create functor
    else if (res.func) {
      node->id_kind = NodeIdentifierKind::ID_Func;

      node->nd_id_target = res.func;

      todo_impl;
    }

    //
    // enum name
    //   => Create type-info of enum
    else if (res.nd_enum) {
      node->id_kind = NodeIdentifierKind::ID_Enum;

      node->nd_id_target = res.nd_enum;

      TypeInfo ti = TypeKind::Type;

      ti.nd_enum = res.nd_enum;

      return ti;
    }

    Error(res.err_id, "cannot find name '" + res.name + "'").crash();
  }

  case ND_Not:
    todo_impl;
    break;

  case ND_Ref:
    todo_impl;
    break;

  case ND_Subscript:
    todo_impl;
    break;

  case ND_MemberAccess:
    todo_impl;
    break;

  case ND_CallFunc:
    return this->check_function_call(node);

  default:
    break;
  }

  auto lhs = this->eval_expr_ti(node->nd_lhs);
  auto rhs = this->eval_expr_ti(node->nd_rhs);

  if (!lhs.equals(rhs))
    Error(node->tok, "type mismatch").crash();

  node->nd.tk = lhs.kind;

  switch (node->kind) {
  case ND_Add:
    if (lhs.is(TypeKind::String))
      break;
    // fallthrough

  case ND_Sub:
  case ND_Mul:
  case ND_Div:
    if (!lhs.is_numeric())
      Error(node->tok,
            "cannot use arithmetic operator for type '" + lhs.to_string() + "'")
          .crash();
    break;

  case ND_Mod:
  case ND_BitAnd:
  case ND_BitOr:
  case ND_BitXor:
  case ND_LShift:
  case ND_RShift:
    if (!lhs.is(TypeKind::Int))
      Error(node->tok, "cannot use operator '" + node->tok->str + "' for type '" +
                           lhs.to_string() + "'")
          .crash();
    break;

  case ND_Compare:
    if (!lhs.is_numeric())
      Error(node->tok,
            "comparing objects of type '" + lhs.to_string() + "' is not valid.")
          .crash();
    break;

  case ND_Equal:
    return TypeKind::Bool;

  case ND_Or:
  case ND_And:
    if (!lhs.is(TypeKind::Bool))
      Error(node->tok, "only can use operator 'or' or 'and' for bool type").crash();
    break;

  case ND_Assign:
    break;
  }

  return lhs;
}

// ---------------------------------
//  eval_type_ti
//    evaluate type info.
// ---------------------------------
TypeInfo Sema::eval_type_ti(Node* node) {
  assert(node->is(ND_TypeName));

  auto const& name = node->tok->str;

  TypeInfo ti;

  if ((ti.kind = TypeInfo::get_kind_of_name(name)) == TypeKind::Unknown) {
    // todo: find user-defined

    Error(node->tok, "unknown type name '" + name + "'").crash();
  }

  for (auto&& arg : node->nd_type_template_args)
    ti.template_args.push_back(this->eval_type_ti(arg));

  ti.is_mutable = node->nd_type_is_mut;
  ti.is_reference = node->nd_type_is_ref;

  return ti;
}
