#include <functional>
#include <span>

#include "alert.h"
#include "Utils.h"
#include "TypeInfo.h"

#include "Token.h"
#include "Node.h"

namespace fire {

using TK = TypeKind;

// clang-format off
static Vec<pair<TypeKind, char const*>> const kind_and_name_table = {
  { TK::None,       "none" },
  { TK::Int,        "int" },
  { TK::Float,      "float" },
  { TK::Bool,       "bool" },
  { TK::Char,       "char" },
  { TK::String,     "string" },
  { TK::Vector,     "vector" },
  { TK::Tuple,      "tuple" },
  { TK::Dict,       "dict" },
  { TK::Functor,    "functor" },
  { TK::Enumerator, "enumerator" },
  { TK::Type,       "type" },
  { TK::Instance,   "instance" },
  { TK::Any,        "any" },
};
// clang-format on

TypeInfo TypeInfo::static_none_type{TypeKind::None};

TypeInfo& TypeInfo::append_template_arg(TypeInfo const& ti) {
  return this->tp_args.emplace_back(ti);
}

bool TypeInfo::is(TypeKind k) const {
  return this->kind == k;
}

bool TypeInfo::is(TypeKind k, bool is_mutable, Vec<TypeInfo> tp_args) const {
  return this->kind == k && this->is_mutable == is_mutable &&
         utils::compare_vector(this->tp_args, tp_args,
                               [](TypeInfo const& a, TypeInfo const& b) -> bool {
                                 return a.equals(b);
                               }) == 0;
}

bool TypeInfo::is_numeric() const {
  return this->is(TK::Int) || this->is(TK::Float);
}

bool TypeInfo::is_subscriptable() const {
  return this->is(TK::String) || this->is(TK::Vector);
}

bool TypeInfo::is_template() const {
  return !this->tp_args.empty();
}

bool TypeInfo::equals(TypeInfo const& ti) const {
  if (this->kind != ti.kind)
    return false;

  if (this->nd_enum != ti.nd_enum)
    return false;

  // if (this->is(TypeKind::Enumerator)) {
  //   if (this->enumerator_index != ti.enumerator_index)
  //     return false;
  // }

  if (this->is_mutable != ti.is_mutable)
    return false;

  if (this->is_reference != ti.is_reference)
    return false;

  if (this->tp_args.size() != ti.tp_args.size())
    return false;

  for (size_t i = 0; i < this->tp_args.size(); i++)
    if (!this->tp_args[i].equals(ti.tp_args[i]))
      return false;

  return true;
}

static string get_kind_str_wrap(TypeInfo const* t) {
  switch (t->kind) {
    case TK::Enumerator:
      return t->nd_enum->nd_enum_name->str + "::" +
             t->nd_enum->get_enumerator(t->enumerator_index)->nd_enumerator_name->str;

    case TK::Type: {
      string s;

      if (t->nd_enum)
        s = t->nd_enum->nd_enum_name->str;
      else if (t->nd_struct)
        s = t->nd_struct->nd_struct_name->str;
      else if (t->nd_class)
        s = t->nd_class->nd_class_name->str;
      else
        todo_impl;

      return "<type-info>";
    }

    case TK::Instance:
      if (t->nd_struct)
        return t->nd_struct->nd_struct_name->str;
      else if (t->nd_class)
        return t->nd_class->nd_class_name->str;
      else
        todo_impl;
  }

  return TypeInfo::get_name_of_kind(t->kind);
}

// -----------------------------------------------
//  to_string
// -----------------------------------------------
string TypeInfo::to_string() const {

  string str;

  if (this->is(TK::Functor)) {
    str = "functor<(" +
          utils::join(", ", std::span(this->tp_args).subspan(1, this->tp_args.size() - 1),
                      [](TypeInfo const& t) -> string {
                        return t.to_string();
                      }) +
          ") -> " + this->tp_args[0].to_string() + ">";

    goto _pass_template_args;
  }
  else {
    str = get_kind_str_wrap(this);
  }

  if (this->is_template()) {
    str += "<" +
           utils::join(", ", this->tp_args,
                       [](TypeInfo const& t) -> string {
                         return t.to_string();
                       }) +
           ">";
  }
_pass_template_args:;

  if (this->is_reference)
    str += " ref";

  if (this->is_mutable)
    str += " mut";

  return str;
}

TypeInfo& TypeInfo::set_enum(Node* nd_enum, size_t index) {
  this->nd_enum = nd_enum;
  this->enumerator_index = index;

  return *this;
}

TypeInfo& TypeInfo::set_ftor_bfun(Builtins::BuiltinFunc const* bf) {
  this->ftor_blt = bf;
  return *this;
}

TypeInfo& TypeInfo::set_ftor_node(Node* node) {
  this->ftor_node = node;
  return *this;
}

string TypeInfo::get_name_of_kind(TypeKind kind) {
  for (auto&& [k, s] : kind_and_name_table)
    if (k == kind)
      return s;

  return "<unknown type>";
}

TypeKind TypeInfo::get_kind_of_name(string const& name) {
  for (auto&& [k, s] : kind_and_name_table)
    if (s == name)
      return k;

  return TypeKind::Unknown;
}

Vec<pair<TypeKind, char const*>> const TypeInfo::get_type_name_map() {
  return kind_and_name_table;
}

TypeInfo::TypeInfo(TypeKind kind)
    : kind(kind),
      is_reference(false),
      is_mutable(false) {
}

TypeInfo::TypeInfo(TypeKind kind, Vec<TypeInfo> tp_args, bool is_ref, bool is_mut)
    : kind(kind),
      tp_args(std::move(tp_args)),
      is_reference(is_ref),
      is_mutable(is_mut) {
}

} // namespace fire