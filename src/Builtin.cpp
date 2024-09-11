#include "Builtin.h"

namespace metro {

using namespace builtin;

ObjPointer Function::Call(ObjVector args) const {
  return this->func(std::move(args));
}

// clang-format off
static const MetroBuiltins g_metro_builtins =
MetroBuiltins {
  .functions = {
    Function{},
  }
};
// clang-format on

MetroBuiltins const& get_metro_builtins() {
  return g_metro_builtins;
}

} // namespace metro
