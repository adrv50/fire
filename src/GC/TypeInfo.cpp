#include "GC/TypeInfo.h"

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

} // namespace metro