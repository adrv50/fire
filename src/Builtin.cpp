#include <iostream>
#include <sstream>

#include "Builtin.h"

namespace metro::builtins {

ObjPointer Function::Call(ObjVector args) const {
  return this->func(std::move(args));
}

Function const* find_builtin_func(std::string const& name) {
  auto const& funcs = get_builtin_functions();

  for (auto&& f : funcs) {
    if (f.name == name)
      return &f;
  }

  return nullptr;
}

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

static ObjPointer bfun_import(ObjVector args) {

  return ObjNew<ObjPrimitive>((i64)0);
}

// clang-format off
static const std::vector<Function> g_builtin_functions = {

  { "print",    bfun_print,     0, true },
  { "println",  bfun_println,   0, true },

  { "import",   bfun_import,    1 },

};
// clang-format on

std::vector<builtins::Function> const& get_builtin_functions() {
  return g_builtin_functions;
}

} // namespace metro::builtins
