#pragma once

#include <string>
#include "Object.h"

namespace metro {
namespace builtin {

struct Function {
  using FuncPointer = ObjPointer (*)(ObjVector);

  std::string name;
  std::vector<TypeInfo> arg_types;
  TypeInfo result_type;
  bool is_variable_args; // true = 可変長引数
  FuncPointer func;

  ObjPointer Call(ObjVector args) const;
};

struct Class {
  std::string name;
  std::vector<TypeInfo> member_var_types; // メンバ変数
  std::vector<Function> member_functions;

  static ObjPointer NewInstance();
};

struct Module {
  std::string name;
  std::vector<ASTPtr<AST::Class>> classes;
  std::vector<Function> functions;
};

Function const* find_builtin_func(std::string const& name);
Class const* find_builtin_class(std::string const& name);
Module const* find_builtin_module(std::string const& name);

} // namespace builtin

struct MetroBuiltins {
  std::vector<builtin::Function> functions;
  std::vector<builtin::Class> classes;
  std::vector<builtin::Module> modules;
};

MetroBuiltins const& get_metro_builtins();

} // namespace metro