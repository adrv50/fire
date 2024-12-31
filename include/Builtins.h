#pragma once

#include <functional>
#include "Object.h"

class Evaluator;

namespace Builtins {

// todo:
//   builtin namespace

struct BuiltinFunc {
  using Impl = std::function<Obj(Evaluator&, Vec<Obj>&)>;

  string name;

  Vec<TypeInfo> arg_types;
  bool is_variable_args = false;

  TypeInfo ret_type;

  Impl impl;

  Obj call(Evaluator& eval, Vec<Obj>& args) const;

  string to_string() const;

  BuiltinFunc(string name, Vec<TypeInfo> arg_types, bool is_variable_args,
              TypeInfo ret_type, Impl impl)
      : name(name),
        arg_types(arg_types),
        is_variable_args(is_variable_args),
        ret_type(ret_type),
        impl(impl) {
  }
};

Vec<BuiltinFunc> const& get_builtin_functions();

} // namespace Builtins