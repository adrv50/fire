#pragma once

#include <functional>
#include "Object.h"

class Evaluator;

namespace Builtins {

// todo:
//   builtin namespace

struct BuiltinFunc {
  using Impl = std::function<Obj(Evaluator&, Vec<Obj> const&)>;

  string name;

  bool is_method; // if method of any type
  TypeInfo self_type;

  Vec<TypeInfo> arg_types;
  bool is_variable_args = false;

  TypeInfo ret_type;

  Impl impl;

  Obj call(Evaluator& eval, Vec<Obj> const& args) const;

  string to_string() const;

  // static BuiltinFunc const* find(string const& name);
  // static BuiltinFunc const* find_method(TypeInfo const& self, string const& name);

  static size_t find(Vec<BuiltinFunc const*>& out, string const& name);

  // ctor for method
  BuiltinFunc(string name, TypeInfo const& self_type, Vec<TypeInfo> arg_types,
              bool is_variable_args, TypeInfo ret_type, Impl impl);

  // ctor for normal function
  BuiltinFunc(string name, Vec<TypeInfo> arg_types, bool is_variable_args,
              TypeInfo ret_type, Impl impl);
};

} // namespace Builtins