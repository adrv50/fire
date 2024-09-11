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

  Module,
  Function,
};

struct TypeInfo {
  TypeKind kind;
  std::vector<TypeInfo> params;

  static std::vector<char const*> get_primitive_names();

  static bool is_primitive_name(std::string_view);

  TypeInfo(TypeKind kind = TypeKind::None) : TypeInfo(kind, {}) {
  }

  TypeInfo(TypeKind kind, std::vector<TypeInfo> params)
      : kind(kind), params(std::move(params)) {
  }
};

} // namespace metro