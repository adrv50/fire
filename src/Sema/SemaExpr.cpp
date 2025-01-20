#include "Object.h"
#include "Driver/Error.h"
#include "Builtins.h"
#include "Sema/Sema.h"

#include "Node/node2s.h"

namespace fire::sema {

ExprEval::ExprEval(Sema& S)
    : S(S) {
}

void ExprEval::save() {
  this->_saves.push_back(this->ctx);
  this->ctx_patch_counter = 0;
}

void ExprEval::restore() {
  this->ctx = this->_saves.back();
  this->_saves.pop_back();
}

void ExprEval::reset() {
  this->ctx = {};
}

TypeInfo ExprEval::handle_call_constructor(Node* node) {
  this->save();
  this->ctx.left_of_call_ctor_expr = true;
  this->ctx.callctor = node;
  this->ctx.callctor_left = node->nd_callctor_ctor_side;

  auto left = this->eval(node->nd_callctor_ctor_side);

  this->restore();

  if (left.is_enumerator())
    return this->handle_construct_enumerator(left, node);

  if (left.is_struct_type())
    return this->handle_construct_struct(left, node);

  if (left.is_class_type())
    return this->handle_construct_class(left, node);

  Error(node->nd_callctor_ctor_side->first_tok,
        "expected name of class or struct or enumerator")
      .crash();
}

TypeInfo ExprEval::handle_construct_enumerator(TypeInfo const& enumerator_ti,
                                               Node* node) {
  Node* enumerator_def = enumerator_ti.sym->decl;

  debug assert(enumerator_def->is(ND_DefEnumerator) ||
               enumerator_def->is(ND_DefEnumeratorWithValue) ||
               enumerator_def->is(ND_DefEnumeratorWithStructFields));

  debug assert(enumerator_ti.nd_enum);
  debug assert(enumerator_ti.nd_enum->is(ND_Enum));

  Vec<Node*>& members = enumerator_ti.get_enumerator_def()->nd_struct_members;
  size_t index = 0;

  auto name = enumerator_ti.to_string();

  for (auto&& pair : node->nd_callctor_initializers) {
    auto const& mb_name = pair->nd_callctor_init_key->str;
    auto mb_init = pair->nd_callctor_init_value;

    if (index >= members.size()) {
      Error(node->nd_callctor_ctor_side,
            "too many initializers to construct instance of '" + name + "'")
          .crash();
    }

    auto mb = members[index];

    Token* name_tok = mb->nd_struct_member_name;
    Node* mb_type = mb->nd_struct_member_type;
    string const& name = name_tok->str;

    if (mb_name != name) {
      Error(pair->tok, "no match member name (index=" + std::to_string(index) + ")")
          .add_cursor_text(name)
          .add_note(name_tok, "defined here")
          .crash();
    }

    this->expect(mb_init, S.eval_type_ti(mb_type));

    index++;
  }

  if (index < members.size()) {
    Error(node->nd_callctor_ctor_side,
          "too few initializers to construct instance of '" + name + "'")
        .crash();
  }

  node->kind = ND_ConstructEnumeratorStruct;
  node->nd_construct_enumerator_enum_def = enumerator_ti.sym->get_parent_scope()->node;
  node->nd_construct_enumerator_index = enumerator_ti.enumerator_index;

  return enumerator_ti;
}

TypeInfo ExprEval::handle_construct_struct(TypeInfo const& struct_ti, Node* node) {

  debug assert(struct_ti.is_struct_type());

  Node* def = struct_ti.nd_class;

  node->nd_callctor_referenced_def = def;

  Vec<Node*>& members = def->nd_struct_members;

  auto mb_end = members.end();
  size_t index = 0;

  auto name = node2s(node->nd_callctor_ctor_side);

  for (auto&& pair : node->nd_callctor_initializers) {
    auto const& mb_name = pair->nd_callctor_init_key->str;
    auto mb_init = pair->nd_callctor_init_value;

    if (index >= members.size()) {
      Error(node->nd_callctor_ctor_side,
            "too many initializers to construct instance of '" + name + "'")
          .crash();
    }

    auto mb = members[index];

    Token* name_tok = mb->nd_struct_member_name;
    Node* mb_type = mb->nd_struct_member_type;
    string const& name = name_tok->str;

    if (mb_name != name) {
      Error(pair->tok, "no match member name (index=" + std::to_string(index) + ")")
          .add_cursor_text(name)
          .add_note(name_tok, "defined here")
          .crash();
    }

    this->expect(mb_init, S.eval_type_ti(mb_type));

    index++;
  }

  if (index < members.size()) {
    Error(node->nd_callctor_ctor_side,
          "too few initializers to construct instance of '" + name + "'")
        .crash();
  }

  auto result = struct_ti;

  result.kind = TypeKind::Instance;

  return result;
}

TypeInfo ExprEval::handle_construct_class(TypeInfo const& class_ti, Node* node) {

  debug assert(class_ti.is_class_type());

  node->nd_callctor_referenced_def = class_ti.nd_class;

  Node* class_def = class_ti.nd_class;

  Vec<Node*>& fields = class_def->nd_class_fields->list;

  auto mb_end = fields.end();
  size_t index = 0;

  auto class_name_str = node2s(node->nd_callctor_ctor_side);

  for (auto&& pair : node->nd_callctor_initializers) {
    auto const& mb_name = pair->nd_callctor_init_key->str;
    auto mb_init = pair->nd_callctor_init_value;

    if (index >= fields.size()) {
      Error(node->nd_callctor_ctor_side,
            "too many initializers to construct instance of '" + class_name_str + "'")
          .crash();
    }

    auto& mb = fields[index];

    Token* name_tok = mb->nd_let_name;
    Node* mb_type = mb->nd_let_type;
    string const& name = name_tok->str;

    if (mb_name != name) {
      Error(pair->tok, "no match member name (index=" + std::to_string(index) + ")")
          .add_cursor_text(name)
          .add_note(name_tok, "defined here")
          .crash();
    }

    this->expect(mb_init, S.eval_type_ti(mb_type));

    index++;
  }

  if (index < fields.size()) {
    Error(node->nd_callctor_ctor_side,
          "too few initializers to construct instance of '" + class_name_str + "'")
        .crash();
  }

  auto result = class_ti;

  result.kind = TypeKind::Instance;

  return result;
}

TypeInfo ExprEval::eval(Node* node) {
  if (!node)
    return TypeKind::None;

  if (node->_is_type_evaluated)
    return node->evaluated_type;

  TypeInfo& result = node->evaluated_type;

  if (++this->ctx_patch_counter == 2) {
    this->reset();
  }

  switch (node->kind) {
    case ND_Value:
      result = node->nd_value->ti;
      break;

    case ND_Identifier:
    case ND_ScopeResol: {
      Vec<Symbol*> candidates;

      Node* id = node->is(ND_ScopeResol) ? node->nd_scope_resol_first : node;

      string name = id->nd_id_name->str;

      ScopeContext* start = nullptr;

      bool flag_mb_ac = this->ctx.in_right_of_member_access;

      if (flag_mb_ac) {
        auto inst = this->ctx.mb_ac_evaluated_left_type;

        start = (inst->nd_class ? inst->nd_class : inst->nd_struct)->sema_ctx->scope;

        assert(start);
      }

      size_t count = S.find_name(candidates, name, start, flag_mb_ac);

      if (count == 0) {
        Error(node,
              flag_mb_ac
                  ? ("couldn't find member or method '" + name + "' in " +
                     string(this->ctx.mb_ac_evaluated_left_type->nd_struct ? "struct"
                                                                           : "class") +
                     " '" + this->ctx.mb_ac_evaluated_left_type->to_string() + "'")
                  : "use of undefined name '" + name + "'")
            .crash();
      }

      if (count >= 2) {
        todo_impl;
      }

      if (node->is(ND_ScopeResol)) {
        auto sr = node->nd_scope_resol_idlist;

        Symbol* sym = nullptr;

        for (auto&& sub : sr) {
          id = sub;

          sym = candidates[0];

          switch (sym->kind) {
            case SY_Class:
              if (sym->decl->nd_class_is_template) {
                alertmsg("not implemented: instantiate class template");
                todo_impl; // instantiate class template
              }

            case SY_Enum:
            case SY_Namespace:
              candidates.clear();
              count = sym->scope->sym_table.find(candidates, id->nd_id_name->str);
              break;

            default:
              Error(sub->first_tok->prev,
                    "'" + name + "' is not enum or class or namespace")
                  .crash();
          }

          name += "::" + id->nd_id_name->str;

          if (candidates.empty()) {
            todo_impl;
          }
          else if (candidates.size() >= 2) {
            todo_impl;
          }
        }
      }

      // if (count == 0) {

      //   // todo:
      //   // find builtin type or func

      //   Error(node, "use of undefined name '" + name + "'").crash();
      // }

      auto sym = candidates[0];

      node->sym = sym;

      switch (sym->kind) {
        case SY_Var: {
          if (!sym->var->is_type_deducted) {
            Error(node, "cannot use variable before type deduction").crash();
          }

          if (id->nd_id_tp_args.size() >= 1) {
            Error(node, "variable '" + name + "' is not template").crash();
          }

          node->kind = ND_Variable;
          node->nd_variable_offset = sym->var->offset_in_stack;
          node->nd_variable_is_global = (sym->get_parent_scope() == this->S.root_scope);

          result = sym->var->type;
          break;
        }

        case SY_Method:
        case SY_Func: {
          TypeInfo type{TypeKind::Functor};

          auto func_nd = sym->decl;

          if (func_nd->nd_func_is_template) {
            Vec<TypeInfo> tp_args;

            for (auto&& arg : id->nd_id_tp_args)
              tp_args.emplace_back(this->eval(arg));

            auto ir = func_nd->sema_ctx->func->template_ir;

            auto instantiated = S.tp_manager.instantiate(
                ir, id, tp_args,
                this->ctx.callfunc_args_p ? *this->ctx.callfunc_args_p : Vec<TypeInfo>(),
                this->ctx.callfunc_nd);

            func_nd = instantiated->node;

            func_nd->nd_func_is_template = false;
            func_nd->nd_func_tplist = nullptr;
          }

          type.ftor_node = func_nd;

          type.append_template_arg(S.eval_type_ti(type.ftor_node->nd_func_result_type));

          for (auto&& arg : type.ftor_node->nd_func_args)
            type.append_template_arg(S.eval_type_ti(arg->nd_func_arg_type));

          node->kind = ND_Functor;

          result = type;
          break;
        }

        case SY_Struct: {

          TypeInfo type{TypeKind::Type};

          type.nd_struct = sym->decl;

          result = type;
          break;
        }

        case SY_Class: {

          TypeInfo type{TypeKind::Type};

          type.nd_class = sym->decl;

          result = type;
          break;
        }

        case SY_BuiltinFunc: {
          TypeInfo type = TypeKind::Functor;

          type.ftor_blt = sym->bfun;

          type.tp_args = sym->bfun->arg_types;
          type.tp_args.insert(type.tp_args.begin(), sym->bfun->ret_type);

          result = type;
          break;
        }

        //
        // when: this->ctx.in_right_of_member_access
        case SY_StructMember: {
          result = this->S.eval_type_ti(sym->decl->nd_struct_member_type);

          this->ctx.mb_ac_node->nd_member_access_index = sym->index_in_table;

          break;
        }

        case SY_Member: {
          result = this->S.eval_type_ti(sym->decl->nd_let_type);

          this->ctx.mb_ac_node->nd_member_access_index = sym->index_in_table;

          break;
        }

        case SY_Enumerator: {
          result = TypeInfo(TypeKind::Enumerator)
                       .set_enum(sym->get_parent_scope()->node, sym->index_in_table);

          assert(sym->get_parent_scope()->node->is(ND_Enum));

          node->kind = ND_EnumeratorName;
          node->nd_enumerator_name_tok = id->tok;
          node->nd_enumerator_enum_node = sym->get_parent_scope()->node;
          node->nd_enumerator_index = sym->index_in_table;

          if (sym->decl->nd_enumerator_is_value) {
            if (!this->ctx.as_functor) {
              Error(node->first_tok, "cannot use '" + name + "' without initializer")
                  .add_note(sym->decl->tok, "declared here")
                  .crash();
            }
          }

          break;
        }

        default:
          Error(node->first_tok, "'" + name + "' is not a variable").crash();
      }

      result.sym = sym;

      break;
    }

    case ND_CallConstructor:
      result = this->handle_call_constructor(node);
      break;

    case ND_Array: {
      if (node->nd_elements.empty()) {
        if (!ctx.allowed_empty_array)
          Error(node, "cannot deduct type of empty array").crash();

        if (!ctx.evaluated_array_type->is(TypeKind::Vector)) {
          auto s = ctx.evaluated_array_type->to_string();

          Error(ctx.array_type_decl,
                "expected 'vector<...>' because array expression are used in "
                "initializer, but found '" +
                    s + "'")
              .add_cursor_text("did you mean 'vector<" + s + ">' ?")
              .crash();
        }

        result = *ctx.evaluated_array_type;
        break;
      }

      auto it = node->nd_elements.begin();
      auto type = this->eval(*it);

      for (++it; it != node->nd_elements.end(); it++)
        this->expect(*it, type);

      result = TypeInfo(TypeKind::Vector, {type});
      break;
    }

    case ND_Tuple: {
      TypeInfo type = TypeKind::Tuple;

      for (auto&& elem : node->nd_elements)
        type.append_template_arg(this->eval(elem));

      result = type;
      break;
    }

    case ND_Dict: {
      auto it = node->nd_dict_pairs.begin();

      auto key = this->eval((*it)->nd_dict_pair_key);
      auto val = this->eval((*it)->nd_dict_pair_value);

      for (++it; it != node->nd_dict_pairs.end(); it++) {
        this->expect((*it)->nd_dict_pair_key, key);
        this->expect((*it)->nd_dict_pair_value, val);
      }

      result = TypeInfo(TypeKind::Dict, {key, val});
      break;
    }

    case ND_Not:
      result = this->expect(node->nd_lhs, TypeKind::Bool);
      break;

    case ND_Ref:
      todo_impl;

    case ND_Cast:
      todo_impl;

    case ND_Subscript: {
      auto arr = this->eval(node->nd_lhs);
      auto index = this->eval(node->nd_rhs);

      if (!index.is(TypeKind::Int))
        Error(node->nd_rhs, "indexer must be integer.").crash();

      if (arr.is(TypeKind::String))
        result = TypeKind::Char;
      else if (arr.is(TypeKind::Vector))
        result = arr.tp_args[0];
      else
        Error(node->tok, "'" + arr.to_string() + "' type object is not subscriptable")
            .crash();

      break;
    }

    case ND_MemberAccess: {
      auto left = this->eval(node->nd_lhs);

      if (!left.is(TypeKind::Instance)) {
        // find method of builtin type
        todo_impl;
      }

      assert(left.is_class_or_struct_instance());

      this->save();
      this->ctx.in_right_of_member_access = true;
      this->ctx.mb_ac_node = node;
      this->ctx.mb_ac_left_node = node->nd_lhs;
      this->ctx.mb_ac_evaluated_left_type = &left;

      result = this->eval(node->nd_rhs);

      this->restore();

      break;
    }

    case ND_CallFunc: {

      Vec<TypeInfo> arg_types;

      for (auto&& arg : node->nd_callfunc_args)
        arg_types.push_back(this->eval(arg));

      this->save();
      this->ctx.in_call_func = true;
      this->ctx.as_functor = true;
      this->ctx.callfunc_nd = node;
      this->ctx.callfunc_args_p = &arg_types;

      TypeInfo functor = this->eval(node->nd_callfunc_callee);

      this->restore();

      if (!functor.is_functor()) {

        if (functor.is_enumerator()) {
          Node* enumerator_def = functor.get_enumerator_def();

          if (!enumerator_def->is(ND_DefEnumeratorWithValue)) {
            Error(node->nd_callfunc_callee,
                  "enumerator '" + functor.to_string() + "' cannot have value")
                .crash();
          }

          if (arg_types.size() == 0)
            Error(node, "too few arguments to construct '" + functor.to_string() + "'")
                .crash();
          else if (arg_types.size() >= 2)
            Error(node, "too many arguments to construct '" + functor.to_string() + "'")
                .crash();

          this->expect(node->nd_callfunc_args[0],
                       this->S.eval_type_ti(enumerator_def->nd_enumerator_val_type));

          node->kind = ND_ConstructEnumeratorValue;
          node->nd_construct_enumerator_enum_def = functor.sym->get_parent_scope()->node;
          node->nd_construct_enumerator_target = enumerator_def;
          node->nd_construct_enumerator_index = functor.sym->index_in_table;
          node->nd_construct_enumerator_arg = node->nd_callfunc_args[0];

          debug assert(node->nd_construct_enumerator_enum_def->is(ND_Enum));

          result = functor;
          break;
        }

        Error(node->nd_callfunc_callee->first_tok, "expected callable expression")
            .crash();
      }

      size_t args_count = arg_types.size();
      size_t args_def_count = functor.tp_args.size() - 1;

      if (args_count < args_def_count) {
        Error(node->nd_callfunc_callee,
              "too few arguments to call '" + functor.to_string() + "'")
            .crash();
      }
      else if (args_count > args_def_count && !functor.is_variable_arg_functor()) {
        Error(node->nd_callfunc_callee,
              "too many arguments to call '" + functor.to_string() + "'")
            .crash();
      }

      if (functor.ftor_node)
        node->nd_callfunc_callee_userdef = functor.ftor_node;
      else
        node->nd_callfunc_callee_builtin = functor.ftor_blt;

      result = functor.tp_args[0];

      break;
    }

    case ND_Range:
      todo_impl;

    case ND_ExprIf: {
      auto type = this->eval(node->nd_if_then);

      this->expect(node->nd_if_cond, TypeKind::Bool);
      this->expect(node->nd_if_else, type);

      result = type;
      break;
    }

    default: {

      assert(node->kind >= ND_Mul && node->kind <= ND_Assign);

      auto lhs = result = this->eval(node->nd_lhs);
      auto rhs = this->eval(node->nd_rhs);

      if (!lhs.equals(rhs)) {
        Error(node->tok, "cannot use operator for not same type ('" + lhs.to_string() +
                             "' and '" + rhs.to_string() + "')")
            .crash();
      }

      switch (node->kind) {
        case ND_Add:
          if (lhs.is_str())
            break;

        case ND_Sub:
        case ND_Mul:
        case ND_Div:
          if (!lhs.is_numeric())
            Error(node->tok, "cannot use arithmetic operator for not numeric type")
                .crash();
          break;

        case ND_LShift:
        case ND_RShift:
        case ND_Mod:
        case ND_BitAnd:
        case ND_BitOr:
        case ND_BitXor:
          if (!lhs.is(TypeKind::Int))
            Error(node->tok,
                  "only can use operator '" + node->tok->str + "' for integer type")
                .crash();
          break;

        case ND_Compare:
          if (!lhs.is_numeric())
            Error(node->tok, "'" + lhs.to_string() + "' type object is not comparable")
                .crash();
          result = TypeKind::Bool;
          break;

        case ND_Equal:
          result = TypeKind::Bool;
          break;

        case ND_In:
          todo_impl;

        case ND_Or:
        case ND_And:
          if (!lhs.is(TypeKind::Bool))
            Error(node->tok, "only can use operator 'or', 'and' for boolean type")
                .crash();

          result = TypeKind::Bool;
          break;

        case ND_Assign:
          break;
      }

      break;
    }
  }

  node->_is_type_evaluated = true;

  return result;
}

TypeInfo ExprEval::expect(Node* node, TypeInfo const& type) {
  if (auto ti = this->eval(node); !ti.equals(type)) {
    Error(node->tok, "expected '" + type.to_string() + "' type expression but found '" +
                         ti.to_string() + "'")
        .crash();
  }
  else
    return ti;
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

} // namespace fire::sema