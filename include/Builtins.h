#pragma once

#include <functional>
#include "typedef.h"
#include "TypeInfo.h"

namespace fire {

class Evaluator;

namespace sema {
struct Symbol;
}

namespace Builtins {

// todo:
//   builtin namespace

struct BuiltinFunc {
  using Impl = std::function<Obj(Evaluator&, Node* node, Vec<Obj> const&)>;

  string name;

  bool is_method; // if method of any type
  TypeInfo self_type;

  Vec<TypeInfo> arg_types;
  bool is_variable_args = false;

  bool is_template = false;
  size_t tp_args_count = 0;

  TypeInfo ret_type;

  Impl impl;

  Obj call(Evaluator& eval, Node* node, Vec<Obj> const& args) const;

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

class Symbols {
public:
  static Vec<fire::sema::Symbol*> const& get_func_symbols();
  static Vec<fire::sema::Symbol*> const& get_type_symbols();

  static size_t find(Vec<fire::sema::Symbol*>& out, string const& name);

private:
  Symbols() = delete;
};

void initialize();

} // namespace Builtins

} // namespace fire