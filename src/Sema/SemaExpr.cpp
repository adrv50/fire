#include "Object.h"
#include "Error.h"
#include "Sema/Sema.h"

#include "node2s.h"

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

void ExprEval::reset() {
  this->ctx = {};
}

TypeInfo ExprEval::eval(Node* node) {
  if (!node)
    return TypeKind::None;

  switch (node->kind) {
    case ND_Value:
      return node->nd_value->ti;

    case ND_Identifier:
    case ND_ScopeResol: {
      Vec<Symbol*> candidates;

      Node* id = node->is(ND_ScopeResol) ? node->nd_scope_resol_first : node;

      string name = id->nd_id_name->str;

      size_t count = S.find_name(candidates, name);

      if (count == 0) {
        Error(node, "use of undefined name '" + name + "'").crash();
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
                todo_impl; // instantiate class template
              }

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

          return sym->var->type;
        }

        case SY_Func: {
          TypeInfo type{TypeKind::Functor};

          type.ftor_node = sym->decl;

          if (sym->decl->nd_func_is_template) {
          }

          type.append_template_arg(S.eval_type_ti(sym->decl->nd_func_result_type));

          for (auto&& arg : sym->decl->nd_func_args)
            type.append_template_arg(S.eval_type_ti(arg->nd_func_arg_type));

          return type;
        }

        case SY_Class: {

          TypeInfo type{TypeKind::Type};

          type.nd_class = sym->decl;

          return type;
        }
      }

      Error(id, "'" + name + "' is not a variable").crash();
    }

    case ND_CallConstructor: {
      auto class_name_ti = this->eval(node->nd_callctor_ctor_side);

      if (!class_name_ti.is_class_type()) {
        Error(node->nd_callctor_ctor_side,
              "'" + node2s(node->nd_callctor_ctor_side) + "' is not name of class")
            .crash();
      }

      auto nd_class = class_name_ti.nd_class;

      auto& fields = nd_class->nd_class_fields->list;

      auto mb_end = fields.end();
      size_t index = 0;

      auto class_name_str = node2s(node->nd_callctor_ctor_side);

      for (auto&& pair : node->nd_callctor_initializers) {
        auto const& mb_name = pair->nd_callctor_init_key->str;
        auto mb_init = pair->nd_callctor_init_value;

        auto mb = fields[index];

        if (mb == *mb_end) {
          Error(node->nd_callctor_ctor_side,
                "too many initializers to construct instance of '" + class_name_str + "'")
              .crash();
        }

        if (mb_name != mb->nd_let_name->str) {
          Error(pair->tok, "no match member name (index=" + std::to_string(index) + ")")
              .add_cursor_text(mb->nd_let_name->str)
              .add_note(mb->nd_let_name, "defined here")
              .crash();
        }

        this->expect(mb_init, S.eval_type_ti(mb->nd_let_type));

        index++;
      }

      if (index < fields.size()) {
        Error(node->nd_callctor_ctor_side,
              "too few initializers to construct instance of '" + class_name_str + "'")
            .crash();
      }

      class_name_ti.kind = TypeKind::Instance;

      return class_name_ti;
    }

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

        return *ctx.evaluated_array_type;
      }

      auto it = node->nd_elements.begin();
      auto type = this->eval(*it);

      for (++it; it != node->nd_elements.end(); it++)
        this->expect(*it, type);

      return TypeInfo(TypeKind::Vector, {type});
    }

    case ND_Tuple: {
      TypeInfo type = TypeKind::Tuple;

      for (auto&& elem : node->nd_elements)
        type.append_template_arg(this->eval(elem));

      return type;
    }

    case ND_Dict: {
      auto it = node->nd_dict_pairs.begin();

      auto key = this->eval((*it)->nd_dict_pair_key);
      auto val = this->eval((*it)->nd_dict_pair_value);

      for (++it; it != node->nd_dict_pairs.end(); it++) {
        this->expect((*it)->nd_dict_pair_key, key);
        this->expect((*it)->nd_dict_pair_value, val);
      }

      return TypeInfo(TypeKind::Dict, {key, val});
    }

    case ND_Not:
      return this->expect(node->nd_lhs, TypeKind::Bool);

    case ND_Ref:
      todo_impl;

    case ND_Cast:
      todo_impl;

    case ND_Subscript: {
      auto arr = this->eval(node->nd_lhs);
      auto index = this->eval(node->nd_rhs);

      if (!index.is(TypeKind::Int)) {
        Error(node->nd_rhs, "indexer must be integer.").crash();
      }

      if (arr.is(TypeKind::String))
        return TypeKind::Char;

      if (!arr.is(TypeKind::Vector))
        Error(node->tok, "'" + arr.to_string() + "' type object is not subscriptable")
            .crash();

      return arr.tp_args[0];
    }

    case ND_MemberAccess: {
      auto left = this->eval(node->nd_lhs);

      if (!left.is(TypeKind::Instance)) {
        // find method of builtin type
        todo_impl;
      }

      assert(left.nd_class);

      todo_impl;

      break;
    }

    case ND_CallFunc: {

      Vec<TypeInfo> arg_types;

      for (auto&& arg : node->nd_callfunc_args)
        arg_types.push_back(this->eval(arg));

      this->ctx.in_call_func = true;
      this->ctx.callfunc_nd = node;
      this->ctx.callfunc_args_p = &arg_types;

      TypeInfo functor = this->eval(node->nd_callfunc_callee);

      this->reset();

      return functor.tp_args[0];
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

TypeInfo ExprEval::expect(Node* node, TypeInfo const& type) {
  if (auto ti = this->eval(node); !ti.equals(type)) {
    Error(node, "expected '" + type.to_string() + "' type expression but found '" +
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