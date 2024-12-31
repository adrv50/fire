#include <iostream>
#include <sstream>

#include "Utils.h"
#include "Builtins.h"

namespace Builtins {

using std::cout;
using std::endl;

//
// b_print
//
static Obj b_print(Evaluator& eval, Vec<Obj>& args) {
  std::stringstream ss;

  for (auto const& arg : args)
    ss << arg->to_string() << " ";

  auto str = ss.str();

  cout << str;

  return ObjInt::make(str.size());
}

//
// b_println
//
static Obj b_println(Evaluator& eval, Vec<Obj>& args) {
  auto res = b_print(eval, args);

  cout << endl;
  res->as_int()->val++;

  return res;
}

static Vec<BuiltinFunc> builtins = {
    // print
    BuiltinFunc("print", {}, true, TypeKind::Int, b_print),

    // println
    BuiltinFunc("println", {}, true, TypeKind::Int, b_println),
};

Obj BuiltinFunc::call(Evaluator& eval, Vec<Obj>& args) const {
  return impl(eval, args);
}

string BuiltinFunc::to_string() const {
  return this->name + "(" +
         utils::join(", ", this->arg_types,
                     [](TypeInfo const& t) -> string {
                       return t.to_string();
                     }) +
         ")";
}

Vec<BuiltinFunc> const& get_builtin_functions() {
  return builtins;
}

} // namespace Builtins