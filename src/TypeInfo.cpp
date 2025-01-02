#include <functional>

#include "alert.h"
#include "Utils.h"
#include "TypeInfo.h"

#include "Token.h"
#include "Node.h"

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

TypeInfo& TypeInfo::append_template_arg(TypeInfo const& ti) {
  return this->template_args.emplace_back(ti);
}

bool TypeInfo::is(TypeKind k) const {
  return this->kind == k;
}

bool TypeInfo::is(TypeKind k, bool is_mutable, Vec<TypeInfo> template_args) const {
  return this->kind == k && this->is_mutable == is_mutable &&
         utils::compare_vector(this->template_args, template_args,
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
  return !this->template_args.empty();
}

bool TypeInfo::equals(TypeInfo const& ti) const {
  if (this->kind != ti.kind)
    return false;

  if (this->is_mutable != ti.is_mutable)
    return false;

  if (this->template_args.size() != ti.template_args.size())
    return false;

  for (size_t i = 0; i < this->template_args.size(); i++)
    if (!this->template_args[i].equals(ti.template_args[i]))
      return false;

  return true;
}

string TypeInfo::to_string() const {
  string str;

  if (this->kind == TK::Enumerator) {
    str = this->nd_enum->nd_enum_enumerators[this->enumerator_index]
              ->nd_enumerator_name->str;
  }
  else if (this->kind == TK::Type) {
    if (this->nd_enum)
      str = this->nd_enum->nd_enum_name->str;
    else
      todo_impl;
  }
  else
    str = get_name_of_kind(this->kind);

  if (this->is_template()) {
    str += "<" +
           utils::join(", ", this->template_args,
                       [](TypeInfo const& t) -> string {
                         return t.to_string();
                       }) +
           ">";
  }

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

string TypeInfo::get_name_of_kind(TypeKind kind) {
  for (auto&& [k, s] : ::kind_and_name_table)
    if (k == kind)
      return s;

  return "<unknown type>";
}

TypeKind TypeInfo::get_kind_of_name(string const& name) {
  for (auto&& [k, s] : ::kind_and_name_table)
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

TypeInfo::TypeInfo(TypeKind kind, Vec<TypeInfo> template_args, bool is_ref, bool is_mut)
    : kind(kind),
      template_args(std::move(template_args)),
      is_reference(is_ref),
      is_mutable(is_mut) {
}