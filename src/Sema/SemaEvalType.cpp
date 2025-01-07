#include <cassert>

#include "alert.h"
#include "Utils.h"

#include "Object.h"
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

  static const auto strargs = [](Vec<TypeInfo> const& _args) -> string {
    return "(" +
           utils::join(", ", _args,
                       [](TypeInfo const& t) -> string {
                         return t.to_string();
                       }) +
           ")";
  };

  (void)strargs;

  TypeInfo self_ti;
  Vec<Node*> fn_candidates;

  Vec<TypeInfo> call_args;

  for (auto&& arg : call->nd_callfunc_args)
    call_args.push_back(this->eval_expr_ti(arg));

  EvalContext E{
      .call_func = call,
      .cf_args_list_ptr = &call_args,
      .fn_candidates_out = &fn_candidates,
  };

  static auto _keep = this->ctx.evalctx;

  this->ctx.evalctx = &E;

  //
  // if method call
  //  => make context
  if (call->nd_callfunc_is_method_call) {
    self_ti = this->eval_expr_ti(call->nd_callfunc_method_self);

    E.method_self = call->nd_callfunc_method_self;
    E.method_self_ti = &self_ti;
  }

  auto callee_ti = this->eval_expr_ti(call->nd_callfunc_callee, &E);

  this->ctx.evalctx = _keep;

  switch (callee_ti.kind) {
    case TypeKind::Functor: {
      if (callee_ti.ftor_blt) {
        call->nd_callfunc_callee_builtin = callee_ti.ftor_blt;
        return callee_ti.template_args[0];
      }

      assert(callee_ti.ftor_node);

      call->nd_callfunc_callee_userdef = callee_ti.ftor_node;

      return *((TypeInfo*)callee_ti.ftor_node->nd_func_result_ti);
    }

    case TypeKind::Enumerator: {

      auto nd_enumerator = callee_ti.nd_enum->get_enumerator(callee_ti.enumerator_index);

      if (nd_enumerator->nd_enumerator_is_value) {
        if (call_args.empty())
          Error(call->tok, "expected 1 argument, but found 0").crash();
        else if (call_args.size() > 1)
          Error(call->tok, "too many arguments to construct enumerator '" +
                               callee_ti.nd_enum->nd_enum_name->str +
                               "::" + nd_enumerator->tok->str + "'")
              .crash();

        auto expected_type = this->eval_type_ti(nd_enumerator->nd_enumerator_val_type);

        if (!expected_type.equals(call_args[0]))
          Error(call->tok, "expected '" + expected_type.to_string() +
                               " type expression, but found '" +
                               call_args[0].to_string() + "'")
              .crash();

        call->kind = ND_ConstructEnumeratorValue;

        call->nd_callfunc_enum_ctor_enum = callee_ti.nd_enum;
        call->nd_callfunc_enum_ctor_index = callee_ti.enumerator_index;

        return callee_ti;
      }

      else {
        todo_impl;
      }

      break;
    }

    default:
      Error(call->tok, "expected callable type expression, but found '" +
                           callee_ti.to_string() + "'")
          .crash();
      break;
  }
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
//  eval_expr_ti:
//    evaluate expression type info.
// ---------------------------------
TypeInfo Sema::eval_expr_ti(Node* node, EvalContext* evalctx) {

  switch (node->kind) {

    // value
    case ND_Value:
      return node->obj->ti;

    // array
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

    //
    // tuple
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

    //
    // dict
    case ND_Dict: {
      return TypeInfo(TypeKind::Dict, {this->eval_expr_ti(node->nd_dict_pair_key),
                                       this->eval_expr_ti(node->nd_dict_pair_value)});
    }

    //
    // ---------
    // identifier
    // scope resolution
    //
    case ND_Identifier:
    case ND_ScopeResol: {

      auto res = node->is(ND_Identifier)
                     ? this->find_name(node, this->get_cur_scope(), false, true)
                     : this->scope_resolution(node);

      TypeInfo result;

      switch (res.type) {

        //
        // Variable
        case NameFindResult::NA_Var: {
          if (!res.var->is_type_deducted)
            Error(node, "cannot use variable before type deducted").crash();

          node->id_kind = NodeIdentifierKind::ID_Var;

          node->nd_variable_offset = res.var->index;
          node->nd_variable_is_global = (res.scope == this->root_scope);

          node->nd_id_target = res.var->decl;

          result = res.var->ti;
          break;
        }

        //
        // function name
        //   => Create functor
        case NameFindResult::NA_Func: {

          ///
          /// if in context of call-function
          if (evalctx && evalctx->call_func) {
            ///
            /// limit candidates (Not erased)
            auto iter = std::remove_if(
                res.fn_candidates.begin(), res.fn_candidates.end(),
                [&](Scope* scope) -> bool {
                  auto cd = scope->node;

                  auto res = this->compare_type_list(
                      *((Vec<TypeInfo>*)cd->nd_func_args_ti), *evalctx->cf_args_list_ptr);

                  return (res & TLC_Many) ? !cd->nd_func_is_variable_args
                                          : (res & TLC_Few) || (res & TLC_Mismatch);
                });

            ///
            /// if no candidates:
            ///  => Error
            if (iter == res.fn_candidates.begin()) {
              Error e{node, "mismatched arguments to call function '" +
                                this->get_full_name(node) + "'"};

              if (res.fn_candidates.size() > 1)
                for (auto&& cd : res.fn_candidates)
                  e.add_note(cd->node->tok, "candidate:");
              else
                e.add_note(res.fn_candidates[0]->node->tok, "defined here");

              e.crash();
            }
          }

          if (res.fn_candidates.size() >= 2) {
            Error e{node, "ambiguous function name '" + res.name + "'"};

            for (auto&& cd : res.fn_candidates)
              e.add_note(cd->node->tok, "candidate:");

            e.crash();
          }

          auto fn = res.fn_candidates[0];

          node->id_kind = NodeIdentifierKind::ID_Func;

          node->nd_id_target = fn->node;

          result = make_functor_ti(fn->arg_types,
                                   this->eval_type_ti(fn->node->nd_func_result_type))
                       .set_ftor_node(fn->node);

          break;
        }

        //
        // enum name
        //   => Create type-info of enum
        case NameFindResult::NA_Enum: {
          node->id_kind = NodeIdentifierKind::ID_Enum;

          node->nd_id_target = res.nd_enum;

          TypeInfo ti = TypeKind::Type;

          ti.nd_enum = res.nd_enum;

          result = ti;
          break;
        }

        //
        // enumerator
        case NameFindResult::NA_Enumerator: {
          node->id_kind = NodeIdentifierKind::ID_Enumerator;

          node->nd_id_target = res.nd_enum;

          node->nd_id_enumerator_index = res.enumerator_index;

          if (res.nd_enum->get_enumerator(res.enumerator_index)->nd_enumerator_is_value &&
              (!evalctx || !evalctx->call_func)) {
            Error(node, "cannot use enumerator '" + this->get_full_name(node) +
                            "' without initializer")
                .crash();
          }

          result =
              TypeInfo(TypeKind::Enumerator).set_enum(res.nd_enum, res.enumerator_index);
          break;
        }

        case NameFindResult::NA_PrimitiveType:
          return res.primitive;

        default: {
          // no found
          // => find builtin function

          if (node->is(ND_ScopeResol))
            break;

          Vec<Builtins::BuiltinFunc const*> bfs;

          size_t const found = Builtins::BuiltinFunc::find(bfs, res.name);

          if (found == 0) {
            Error(res.err_id, "cannot find name '" + this->get_full_name(node) + "'")
                .append_msg_if([&](string& msg) {
                  if (evalctx && evalctx->method_self_ti)
                    msg += " in type '" + evalctx->method_self_ti->to_string() + "'";
                })
                .crash();
          }

          if (found >= 2) {
            if (evalctx && evalctx->call_func) {
              todo_impl; // limit candidates
            }
            else
              Error(node, "ambiguous builtin-function name '" + res.name + "'").crash();
          }

          result = Sema::make_functor_ti(bfs[0]->arg_types, bfs[0]->ret_type)
                       .set_ftor_bfun(bfs[0]);
        }
      }

      return result;
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

  node->tk = lhs.kind;

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

    if (auto tpi = this->find_template_param(name)) {
      assert(tpi->is_deducted);

      return tpi->type;
    }

    Error(node->tok, "unknown type name '" + name + "'").crash();
  }

  for (auto&& arg : node->nd_type_template_args)
    ti.template_args.push_back(this->eval_type_ti(arg));

  ti.is_mutable = node->nd_type_is_mut;
  ti.is_reference = node->nd_type_is_ref;

  return ti;
}

// ---------------------------------
//  eval_as_functor
//    evaluate node as functor.
// ---------------------------------
Sema::FunctorEvalResult Sema::eval_as_functor(Node* node) {

  if (node->is_id_or_sr()) {
    auto res = this->find_name(node, this->get_cur_scope(), false, true);

    if (res.func)
      return FunctorEvalResult(node, res.func, this->eval_expr_ti(node));
  }

  auto ti = this->eval_expr_ti(node);

  if (ti.is(TypeKind::Functor)) {
    todo_impl;
  }

  else {
    Error(node->tok,
          "expected callable type expression, but found '" + ti.to_string() + "'")
        .crash();
  }

  return FunctorEvalResult(node, nullptr, ti);
}
