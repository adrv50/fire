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
      Vec<Symbol*> candidates;

      auto const& name = node->nd_id_name->str;

      auto count = S.find_name(candidates, name);

      if (count >= 2) {
        todo_impl;
      }

      if (count == 0) {

        // todo:
        // find builtin type or func

        Error(node, "use of undefined name '" + name + "'").crash();
      }

      auto sym = candidates[0];

      switch (sym->kind) {
        case SY_Var: {
          if (!sym->var->is_type_deducted) {
            Error(node, "cannot use variable before type deduction").crash();
          }

          return sym->var->type;
        }

        case SY_Func: {
          todo_impl;
        }

        case SY_Class: {

          TypeInfo type{TypeKind::Type};

          type.nd_class = sym->decl;

          return type;
        }
      }

      todo_impl;
    }

    case ND_CallConstructor: {
      todo_impl;
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

      if (!arr.is(TypeKind::Vector))
        Error(node->tok, "'" + arr.to_string() + "' type object is not subscriptable")
            .crash();

      if (!index.is(TypeKind::Int)) {
        Error(node->nd_rhs, "indexer must be integer.").crash();
      }

      return arr.tp_args[0];
    }

    case ND_MemberAccess: {
      auto left = this->eval(node->nd_lhs);

      if (left.is(TypeKind::Instance)) {
      }

      todo_impl;
    }

    case ND_CallFunc: {

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