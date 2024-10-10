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

  // when enumerator
  size_t enum_index = 0;

  //
  // TypeKind::Function
  bool is_free_args = false;
  bool is_member_func = false;

  //
  //   0  = no need
  //  -1  = infinity
  // >=1  =
  int needed_param_count() const;

  bool IsPrimitiveType() const {
    switch (this->kind) {
    case TypeKind::None:
    case TypeKind::Int:
    case TypeKind::Float:
    case TypeKind::Bool:
    case TypeKind::Char:
    case TypeKind::String:
    case TypeKind::Vector:
    case TypeKind::Tuple:
    case TypeKind::Dict:
      return true;
    }

    return false;
  }

  bool is_iterable() const;

  bool is_numeric() const;
  bool is_numeric_or_char() const;

  bool is_char_or_str() const;

  bool is_hit(std::vector<TypeInfo> types) const;
  bool is_hit_kind(std::vector<TypeKind> kinds) const;

  string_view GetSV() const;

  static TypeInfo from_enum(ASTPtr<AST::Enum> ast);
  static TypeInfo from_class(ASTPtr<AST::Class> ast);

  static TypeInfo make_instance_type(ASTPtr<AST::Class> ast);

  static TypeKind from_name(string const& name);

  static TypeKind from_name(string_view const& name) {
    return from_name(string(name));
  }

  static bool is_primitive_name(std::string_view);

  bool equals(TypeInfo const& type) const;
  std::string to_string() const;

  TypeInfo without_params() const;

  TypeInfo(TypeKind kind = TypeKind::None);
  TypeInfo(TypeKind kind, std::vector<TypeInfo> params);
};

} // namespace fire