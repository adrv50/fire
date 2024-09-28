#pragma once

#include <vector>
#include "types.h"

namespace fire {

enum class TypeKind : u8 {
  None,

  Int,
  Float,
  Bool,

  Char,
  String,

  Vector,
  Tuple,
  Dict,

  Enumerator,
  Instance, // instance of class

  //
  // Function:
  //   params[  0]  = result
  //   params[>=1]  = args
  Function,

  Module,

  TypeName, // class or enum or etc...

  Unknown, // or template param
};

struct TypeInfo {
  TypeKind kind;
  std::vector<TypeInfo> params;

  std::string name;

  bool is_const = false;

  // TypeKind::TypeName
  //
  // AST::Enum
  // AST::Class
  ASTPointer type_ast = nullptr;

  //
  //   0  = no need
  //  -1  = infinity
  // >=1  =
  int needed_param_count() const;

  bool is_numeric() const {
    switch (this->kind) {
    case TypeKind::Int:
    case TypeKind::Float:
      return true;
    }

    return false;
  }

  bool is_numeric_or_char() const {
    return this->is_numeric() || this->kind == TypeKind::Char;
  }

  bool is_char_or_str() const {
    return this->kind == TypeKind::Char || this->kind == TypeKind::String;
  }

  bool is_hit(std::vector<TypeInfo> types) const {
    for (auto&& _t : types)
      if (this->equals(_t))
        return true;

    return false;
  }

  static TypeInfo from_enum(ASTPtr<AST::Enum> ast);
  static TypeInfo from_class(ASTPtr<AST::Class> ast);

  static std::vector<char const*> get_primitive_names();

  static bool is_primitive_name(std::string_view);

  bool equals(TypeInfo const& type) const;
  std::string to_string() const;

  TypeInfo without_params() const;

  TypeInfo(TypeKind kind = TypeKind::None)
      : TypeInfo(kind, {}) {
  }

  TypeInfo(TypeKind kind, std::vector<TypeInfo> params)
      : kind(kind),
        params(std::move(params)) {
  }
};

} // namespace fire