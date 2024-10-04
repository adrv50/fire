#include <map>
#include <cassert>

#include "alert.h"
#include "Utils.h"
#include "TypeInfo.h"
#include "AST.h"

namespace fire {

// clang-format off
static char const* g_names[] = {
  "none",

  "int",
  "float",
  "bool",
  
  "char",
  "string",
  
  "vector",
  "tuple",
  "dict",

  "", // Enumerator
  "", // Instance

  "function", // Function
  "", // Module

  "", // TypeName
  "", // Unknown
};


debug(
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
}
);
// clang-format on

bool TypeInfo::is_numeric() const {
  switch (this->kind) {
  case TypeKind::Int:
  case TypeKind::Float:
    return true;
  }

  return false;
}

bool TypeInfo::is_numeric_or_char() const {
  return this->is_numeric() || this->kind == TypeKind::Char;
}

bool TypeInfo::is_char_or_str() const {
  return this->kind == TypeKind::Char || this->kind == TypeKind::String;
}

bool TypeInfo::is_hit(std::vector<TypeInfo> types) const {
  for (TypeInfo const& t : types)
    if (this->equals(t))
      return true;

  return false;
}

bool TypeInfo::is_hit_kind(std::vector<TypeKind> kinds) const {
  for (TypeKind k : kinds)
    if (this->kind == k)
      return true;

  return false;
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

  if (this->is_const != type.is_const)
    return false;

  if (this->params.size() != type.params.size())
    return false;

  switch (this->kind) {
  case TypeKind::TypeName:
  case TypeKind::Instance:
    if (this->type_ast != type.type_ast)
      return false;
    break;

  case TypeKind::Function:
    if (this->is_free_args != type.is_free_args)
      return false;
    break;
  }

  for (auto it = this->params.begin(); auto&& t : type.params)
    if (!it->equals(t))
      return false;

  return true;
}

string TypeInfo::to_string() const {
  string ret;

  switch (this->kind) {
  case TypeKind::TypeName:
    if (this->params.size() == 1) {
      ret = this->params[0].to_string();
      break;
    }

  case TypeKind::Instance:
  case TypeKind::Enumerator:
  case TypeKind::Unknown:
    ret = this->name;
    break;

  default:
    debug(assert(kind_name_map.contains(this->kind)));
    ret = g_names[static_cast<u8>(this->kind)];
    break;
  }

  if (this->kind == TypeKind::Function) {
    ret += "<(";

    for (auto it = this->params.begin() + 1; it < this->params.end(); it++) {
      ret += it->to_string();
      if (it + 1 != this->params.end())
        ret += ", ";
    }

    if (this->is_free_args)
      ret += "...";

    ret += ") -> " + this->params[0].to_string() + ">";
  }
  else if (!this->params.empty()) {
    ret += "<" +
           utils::join<TypeInfo>(", ", this->params,
                                 [](TypeInfo t) -> string {
                                   return t.to_string();
                                 }) +
           ">";
  }

  if (this->is_const)
    ret += " const";

  if (this->kind == TypeKind::TypeName)
    return "<typeinfo of " + ret + ">";

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

  case TypeKind::Function:
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

bool TypeInfo::is_iterable() const {
  switch (this->kind) {
  case TypeKind::Vector:
    return true;
  }

  return false;
}

TypeInfo TypeInfo::from_enum(ASTPtr<AST::Enum> ast) {
  TypeInfo t = TypeKind::TypeName;

  t.type_ast = ast;
  t.name = ast->GetName();

  return t;
}

TypeInfo TypeInfo::from_class(ASTPtr<AST::Class> ast) {
  TypeInfo t = TypeKind::TypeName;

  t.type_ast = ast;
  t.name = ast->GetName();

  return t;
}

TypeInfo TypeInfo::make_instance_type(ASTPtr<AST::Class> ast) {
  auto ret = from_class(ast);

  ret.kind = TypeKind::Instance;

  return ret;
}

TypeKind TypeInfo::from_name(string const& name) {
  for (int i = 0; auto& s : g_names) {
    if (s == name)
      return static_cast<TypeKind>(i);

    i++;
  }

  return TypeKind::Unknown;
}

TypeInfo::TypeInfo(TypeKind kind)
    : TypeInfo(kind, {}) {
}

TypeInfo::TypeInfo(TypeKind kind, std::vector<TypeInfo> params)
    : kind(kind),
      params(std::move(params)) {
}

} // namespace fire