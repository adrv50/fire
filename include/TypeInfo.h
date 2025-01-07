#pragma once

#include "typedef.h"

namespace Builtins {
struct BuiltinFunc;
}

enum class TypeKind : u8 {
  Unknown,

  None,

  Int,
  Float,
  Bool,
  Char,
  String,

  Vector,
  Tuple,
  Dict,

  Functor,

  Enumerator,

  Type, // => the type of a type. (class, struct, enum, etc...)

  Instance,

  Any,
};

struct Node;
struct TypeInfo {
  TypeKind kind;
  Vec<TypeInfo> template_args;

  bool is_reference;
  bool is_mutable;

  Node* nd_enum = nullptr;
  size_t enumerator_index = 0;

  Node* ftor_node = nullptr; // when TypeKind::Functor, ptr to user-defined function
  Builtins::BuiltinFunc const* ftor_blt = nullptr; // not uder-def but if builtin

  static TypeInfo static_none_type;

  TypeInfo& append_template_arg(TypeInfo const& ti);

  bool is(TypeKind k) const;
  bool is(TypeKind k, bool is_mutable, Vec<TypeInfo> template_args) const;

  bool is_numeric() const;
  bool is_subscriptable() const;
  bool is_template() const;

  bool equals(TypeInfo const& ti) const;

  string to_string() const;

  TypeInfo& set_enum(Node* nd_enum, size_t index = 0);

  TypeInfo& set_ftor_bfun(Builtins::BuiltinFunc const* bf);
  TypeInfo& set_ftor_node(Node* node);

  static string get_name_of_kind(TypeKind kind);

  static TypeKind get_kind_of_name(string const& name);

  static Vec<pair<TypeKind, char const*>> const get_type_name_map();

  TypeInfo& get_elem_type(size_t template_param_index = 0) {
    return this->template_args[template_param_index];
  }

  TypeInfo(TypeKind kind = TypeKind::None);
  TypeInfo(TypeKind kind, Vec<TypeInfo> template_args, bool is_reference = false,
           bool is_mutable = false);
};
