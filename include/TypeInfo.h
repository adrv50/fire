#pragma once

#include <vector>
#include "types.h"

namespace metro {

enum class TypeKind : u8 {
  None,

  Int,
  Float,
  Size,
  Bool,
  Char,

  String,
  Vector,
  Tuple,
  Dict,

  Instance, // instance of class

  Module,
  Function,
};

struct TypeInfo {
  TypeKind kind;
  std::vector<TypeInfo> params;

  bool is_const = false;

  bool is_numeric() const {
    switch (this->kind) {
    case TypeKind::Int:
    case TypeKind::Float:
    case TypeKind::Size:
      return true;
    }

    return false;
  }

  static std::vector<char const*> get_primitive_names();

  static bool is_primitive_name(std::string_view);

  bool equals(TypeInfo const& type) const;
  std::string to_string() const;

  TypeInfo(TypeKind kind = TypeKind::None) : TypeInfo(kind, {}) {
  }

  TypeInfo(TypeKind kind, std::vector<TypeInfo> params)
      : kind(kind), params(std::move(params)) {
  }
};

} // namespace metro