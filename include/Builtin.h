#pragma once

#include <string>
#include "Object.h"

namespace fire::builtins {

struct Function {
  using FuncPointer = ObjPointer (*)(ASTPtr<AST::CallFunc>, ObjVector);

  std::string name;

  TypeInfo result_type;
  vector<TypeInfo> arg_types;

  bool is_variable_args; // true = 可変長引数

  FuncPointer func;

  ObjPointer Call(ASTPtr<AST::CallFunc> ast, ObjVector args) const;

  Function(std::string const& name, FuncPointer fp, TypeInfo result_type,
           vector<TypeInfo> arg_types, bool is_vararg = false)
      : name(name),
        result_type(result_type),
        arg_types(arg_types),
        is_variable_args(is_vararg),
        func(fp) {
  }
};

Function const* find_builtin_func(std::string const& name);

std::vector<builtins::Function> const& get_builtin_functions();

} // namespace fire::builtins