#pragma once

#include <string>
#include "Object.h"

namespace metro::builtins {

struct Function {
  using FuncPointer = ObjPointer (*)(ObjVector);

  std::string name;

  int argcount;          // 引数が (argcount) 以上必要
  bool is_variable_args; // true = 可変長引数

  FuncPointer func;

  ObjPointer Call(ObjVector args) const;

  Function(std::string const& name, FuncPointer fp, int argcount,
           bool is_vararg = false)
      : name(name),
        argcount(argcount),
        is_variable_args(is_vararg),
        func(fp) {
  }
};

Function const* find_builtin_func(std::string const& name);

std::vector<builtins::Function> const& get_builtin_functions();

} // namespace metro::builtins