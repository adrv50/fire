#include "Object.h"
#include "Driver/Error.h"
#include "Builtins.h"
#include "Sema/Sema.h"

#include "Debug/Debug.h"

namespace fire::sema {

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

TypeInfo ExprEval::handle_match_case_cond(Node* node) {
  return {};
}

} // namespace fire::sema