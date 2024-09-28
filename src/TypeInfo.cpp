#include <map>
#include <cassert>

#include "alert.h"
#include "Utils.h"
#include "TypeInfo.h"
#include "AST.h"

namespace fire {

// clang-format off
static std::vector<char const*> g_names{
  "int",
  "float",
  "bool",
  
  "char",
  "string",
  
  "vector",
  "tuple",
  "dict",

  "(enumerator)",
  "(instance)",

  "function",
  "module",

  "(typename)",
  "(unk)",
};
// clang-format on

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
  if (this->kind == TypeKind::Unknown || type.kind == TypeKind::Unknown)
    return true;

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
  case TypeKind::Unknown:
    ret = name;
    break;

  default:
    ret = kind_name_map[this->kind];
  }

  if (!this->params.empty()) {
    ret += "<" +
           utils::join<TypeInfo>(", ", this->params,
                                 [](TypeInfo t) -> string {
                                   return t.to_string();
                                 }) +
           ">";
  }

  if (this->is_const)
    ret += " const";

  return ret;
}

TypeInfo TypeInfo::without_params() const {
  auto copy = *this;

  copy.params.clear();

  return copy;
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
    if (auto x = ASTCast<AST::Templatable>(this->type_ast); x->is_templated) {
      return x->template_param_names.size();
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