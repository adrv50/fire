#include <iostream>
#include <sstream>

#include "Builtin.h"

namespace metro {

namespace builtin {

ObjPointer Function::Call(ObjVector args) const {
  return this->func(std::move(args));
}

Function const* find_builtin_func(std::string const& name) {
  auto const& funcs = get_metro_builtins().functions;

  for (auto&& f : funcs) {
    if (f.name == name)
      return &f;
  }

  return nullptr;
}

} // namespace builtin

static ObjPointer bfun_print(ObjVector args) {
  std::stringstream ss;

  for (auto&& obj : args)
    ss << obj->ToString();

  auto str = ss.str();

  std::cout << str;

  return ObjNew<ObjPrimitive>((i64)str.length());
}

static ObjPointer bfun_println(ObjVector args) {
  auto ret = ObjNew<ObjPrimitive>(
      bfun_print(std::move(args))->As<ObjPrimitive>()->vi + 1);

  std::cout << std::endl;
  return ret;
}

// clang-format off
static const MetroBuiltins g_metro_builtins =
MetroBuiltins {
  .functions = {
    builtin::Function {
      .name = "print",
      .arg_types = { },
      .result_type = TypeKind::Int,
      .is_variable_args = true,
      .func = bfun_print
    },

    builtin::Function {
      .name = "println",
      .arg_types = { },
      .result_type = TypeKind::Int,
      .is_variable_args = true,
      .func = bfun_println
    },
  }
};
// clang-format on

MetroBuiltins const& get_metro_builtins() {
  return g_metro_builtins;
}

} // namespace metro
