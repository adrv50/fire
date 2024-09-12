#include <map>
#include <cassert>

#include "alert.h"
#include "TypeInfo.h"

namespace metro {

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

  return true;
}

std::string TypeInfo::to_string() const {
  // clang-format off
  std::map<TypeKind, char const*> kind_name_map {
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
    { TypeKind::Module,     "module" },
    { TypeKind::Function,   "function" },
  };
  // clang-format on

  debug(assert(kind_name_map.contains(this->kind)));

  std::string ret = kind_name_map[this->kind];

  if (!this->params.empty()) {
    todo_impl;
  }

  if (this->is_const)
    ret += " const";

  return ret;
}

} // namespace metro