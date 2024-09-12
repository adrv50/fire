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

  Function(std::string const& name, std::vector<TypeInfo> arg_types,
           TypeInfo result, bool is_vararg, FuncPointer fp)
      : name(name), arg_types(arg_types), result_type(result),
        is_variable_args(is_vararg), func(fp) {
  }
};

struct Class {
  std::string name;
  std::vector<std::string> member_var_names; // メンバ変数
  std::vector<Function> member_functions;

  //
  // NewInstance
  //
  // メンバ変数の値は Evaluator にて設定する
  ObjPointer NewInstance() const;
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