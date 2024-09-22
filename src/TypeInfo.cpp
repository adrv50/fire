#include <map>
#include <cassert>

#include "alert.h"
#include "TypeInfo.h"
#include "AST.h"

namespace fire {

static std::vector<char const*> g_names{
    "int",    "float", "size", "bool",   "char",     "string",
    "vector", "tuple", "dict", "module", "function",
};

std::vector<char const*> TypeInfo::get_primitive_names() {
  return g_names;
}

bool TypeInfo::is_primitive_name(std::string_view name) {
  for (auto&& s : g_names)
    if (s == name)
      return true;

  return false;
}

bool TypeInfo::equals(TypeInfo const& type) const {
  if (this->kind != type.kind)
    return false;

  if (this->params.size() != type.params.size())
    return false;

  for (auto it = this->params.begin(); auto&& t : type.params)
    if (!it->equals(t))
      return false;

  return true;
}

string TypeInfo::to_string() const {
  // clang-format off
  static std::map<TypeKind, char const*> kind_name_map {
    { TypeKind::None,       "none" },
    { TypeKind::Int,        "int" },
    { TypeKind::Float,      "float" },
    { TypeKind::Size,       "usize" },
    { TypeKind::Bool,       "bool" },
    { TypeKind::Char,       "char" },
    { TypeKind::String,     "string" },
    { TypeKind::Vector,     "vector" },
    { TypeKind::Tuple,      "tuple" },
    { TypeKind::Dict,       "dict" },
    { TypeKind::Instance,   "instance" },
    { TypeKind::Module,     "module" },
    { TypeKind::Function,   "function" },
    { TypeKind::Module,     "module" },
    { TypeKind::TypeName,   "type" },
  };
  // clang-format on

  debug(assert(kind_name_map.contains(this->kind)));

  string ret;

  switch (this->kind) {
  case TypeKind::Instance:
  case TypeKind::Enumerator:
  case TypeKind::TypeName:
    ret = name;
    break;

  default:
    ret = kind_name_map[this->kind];
  }

  if (!this->params.empty()) {
    todo_impl;
  }

  if (this->is_const)
    ret += " const";

  return ret;
}

int TypeInfo::needed_param_count() const {
  switch (this->kind) {
  case TypeKind::Vector:
    return 1;

  case TypeKind::Tuple:
    return -1;

  case TypeKind::Dict:
    return 2; // key, value

  case TypeKind::TypeName: {
    if (auto x = ASTCast<AST::Templatable>(this->type_ast);
        x->is_templated) {
      return x->template_params.size();
    }

    break;
  }
  }

  return 0;
}

TypeInfo TypeInfo::from_enum(ASTPtr<AST::Enum> ast) {
  TypeInfo t = TypeKind::TypeName;

  t.type_ast = ast;

  return t;
}

TypeInfo TypeInfo::from_class(ASTPtr<AST::Class> ast) {
  TypeInfo t = TypeKind::TypeName;

  t.type_ast = ast;

  return t;
}

} // namespace fire