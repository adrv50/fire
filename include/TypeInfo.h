#pragma once

#include "alert.h"
#include "typedef.h"

namespace fire {

namespace Builtins {
struct BuiltinFunc;
}

namespace sema {
struct Symbol;
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
  Vec<TypeInfo> tp_args;

  bool is_reference;
  bool is_mutable;

  sema::Symbol* sym = nullptr;

  Node* nd_enum = nullptr;
  size_t enumerator_index = 0;

  Node* nd_class = nullptr;
  Node* nd_struct = nullptr;

  Node* ftor_node = nullptr; // when TypeKind::Functor, ptr to user-defined function
  Builtins::BuiltinFunc const* ftor_blt = nullptr; // not uder-def but if builtin

  static TypeInfo static_none_type;

  TypeInfo& append_template_arg(TypeInfo const& ti);

  bool is(TypeKind k) const;
  bool is(TypeKind k, bool is_mutable, Vec<TypeInfo> tp_args) const;

  bool is_functor() const;
  bool is_functor_of_method() const;
  bool is_variable_arg_functor() const;

  bool is_str() const {
    return this->is(TypeKind::String);
  }

  bool is_enum_type() const {
    return this->is(TypeKind::Type) && this->nd_enum;
  }

  bool is_struct_type() const {
    return this->is(TypeKind::Type) && this->nd_struct;
  }

  bool is_class_type() const {
    return this->is(TypeKind::Type) && this->nd_class;
  }

  bool is_class_or_struct_type() const {
    return this->is(TypeKind::Type) && (this->nd_struct || this->nd_class);
  }

  bool is_class_or_struct_instance() const {
    return this->is(TypeKind::Instance) && (this->nd_struct || this->nd_class);
  }

  bool is_numeric() const;
  bool is_subscriptable() const;
  bool is_template() const;

  bool is_callable() const {
    debug {
      if (this->is(TypeKind::Functor)) {
        assert(this->ftor_node || this->ftor_blt);
      }
    };

    return this->is(TypeKind::Functor);
  }

  bool equals(TypeInfo const& ti) const;

  string to_string() const;

  TypeInfo& set_enum(Node* nd_enum, size_t index = 0);

  TypeInfo& set_ftor_bfun(Builtins::BuiltinFunc const* bf);
  TypeInfo& set_ftor_node(Node* node);

  static size_t get_least_template_args_count_of(TypeKind kind) {
    switch (kind) {
      case TypeKind::Vector:
        return 1;

      case TypeKind::Tuple:
        return 1;

      case TypeKind::Dict:
        return 2;
    }

    return 0;
  }

  static bool is_template_kind(TypeKind kind) {
    switch (kind) {
      case TypeKind::Vector:
      case TypeKind::Tuple:
      case TypeKind::Dict:
        return true;
    }

    return false;
  }

  static string get_name_of_kind(TypeKind kind);

  static TypeKind get_kind_of_name(string const& name);

  static Vec<pair<TypeKind, char const*>> const get_type_name_map();

  TypeInfo& get_elem_type(size_t template_param_index = 0) {
    return this->tp_args[template_param_index];
  }

  TypeInfo(TypeKind kind = TypeKind::None);
  TypeInfo(TypeKind kind, Vec<TypeInfo> tp_args, bool is_reference = false,
           bool is_mutable = false);
};

} // namespace fire