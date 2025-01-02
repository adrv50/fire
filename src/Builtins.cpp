#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>

#include "Utils.h"

#include "Object.h"
#include "Token.h"
#include "Node.h"
#include "Builtins.h"

#define def_builtin_func(_Name)                                                          \
  static Obj b_##_Name([[maybe_unused]] Evaluator& eval,                                 \
                       [[maybe_unused]] Vec<Obj> const& args)

namespace Builtins {

using std::cout;
using std::endl;

//
// input(prompt: string) -> string
//
def_builtin_func(input) {
  string prompt = args[0]->as_str()->to_string();
  string input;

  cout << prompt;
  std::getline(std::cin, input);

  return ObjStr::make(input);
}

//
// print(...) -> int
//
def_builtin_func(print) {
  std::stringstream ss;

  for (auto const& arg : args)
    ss << arg->to_string() << " ";

  auto str = ss.str();

  cout << str;

  return ObjInt::make(str.length());
}

//
// println(...) -> int
//
def_builtin_func(println) {
  auto res = b_print(eval, args);

  cout << endl;
  res->as_int()->val++;

  return res;
}

//
// exit(int) -> void
//
def_builtin_func(exit) {
  auto res = args[0]->as_int();

  std::exit(static_cast<int>(res->val));
}

//
// string::substr(self, pos: int) -> string
//
def_builtin_func(substr_1) {
  ObjPtr<ObjStr> str = args[0]->as_str();

  i64 start = args[1]->as_int()->val;

  return ObjStr::make(str->val.substr(static_cast<size_t>(start)));
}

//
// string::substr(self, pos: int, len: int = -1) -> string
//
def_builtin_func(substr_2) {
  ObjPtr<ObjStr> str = args[0]->as_str();

  i64 start = args[1]->as_int()->val;
  i64 len = args[2]->as_int()->val;

  if (len == -1)
    len = (i64)str->length() - start;

  return ObjStr::make(
      str->val.substr(static_cast<size_t>(start), static_cast<size_t>(len)));
}

//
// string::length(self) -> int
//
def_builtin_func(length) {
  ObjPtr<ObjStr> str = args[0]->as_str();

  return ObjInt::make(str->length());
}

//
// vector::size(self) -> int
//
def_builtin_func(vector_size) {
  ObjPtr<ObjVector> vec = args[0]->as_vector();

  return ObjInt::make(vec->list.size());
}

//
// vector::append(self, value: any) -> vector
//
def_builtin_func(vector_append) {
  ObjPtr<ObjVector> vec = args[0]->as_vector();

  vec->list.emplace_back(args[1]);

  return vec;
}

//
// float::abs(self) -> float
//
def_builtin_func(float_abs) {
  return ObjFloat::make(std::abs(args[0]->as_float()->val));
}

//
// float::floor(self) -> float
//
def_builtin_func(floor) {
  return ObjFloat::make(std::floor(args[0]->as_float()->val));
}

//
// float::ceil(self) -> float
//
def_builtin_func(ceil) {
  return ObjFloat::make(std::ceil(args[0]->as_float()->val));
}

//
// float::round(self) -> float
//
def_builtin_func(round) {
  return ObjFloat::make(std::round(args[0]->as_float()->val));
}

//
// int::abs(self) -> int
//
def_builtin_func(abs) {
  ObjPtr<ObjInt> i = args[0]->as_int();

  return ObjInt::make(std::abs(i->val));
}

//
// int::pow(self, exp: int) -> int
//
def_builtin_func(pow) {
  ObjPtr<ObjInt> i = args[0]->as_int();
  ObjPtr<ObjInt> exp = args[1]->as_int();

  return ObjInt::make(std::pow(i->val, exp->val));
}

//
// int::fib(self) -> int
//
def_builtin_func(fib) {
  ObjPtr<ObjInt> obj = args[0]->as_int();

  if (auto n = obj->val; n < 2)
    return ObjInt::make(1);
  else
    return ObjInt::make(b_fib(eval, {ObjInt::make(n - 1)})->as_int()->val +
                        b_fib(eval, {ObjInt::make(n - 2)})->as_int()->val);
}

//
// (any)::to_string(self) -> string
//
def_builtin_func(to_string) {
  return ObjStr::make(args[0]->to_string());
}

static BuiltinFunc const builtins[] = {
    // input
    BuiltinFunc("input", {TypeKind::String}, false, TypeKind::String, b_input),

    // print
    BuiltinFunc("print", {}, true, TypeKind::Int, b_print),

    // println
    BuiltinFunc("println", {}, true, TypeKind::Int, b_println),

    // exit
    BuiltinFunc("exit", {TypeKind::Int}, false, TypeKind::None, b_exit),

    // string::substr
    BuiltinFunc("substr", TypeKind::String, {TypeKind::Int}, false, TypeKind::String,
                b_substr_1),

    // string::substr
    BuiltinFunc("substr", TypeKind::String, {TypeKind::Int, TypeKind::Int}, false,
                TypeKind::String, b_substr_2),

    // string::length
    BuiltinFunc("length", TypeKind::String, {}, false, TypeKind::Int, b_length),

    // vector::size
    BuiltinFunc("size", TypeKind::Vector, {}, false, TypeKind::Int, b_vector_size),

    // vector::append
    BuiltinFunc("append", TypeKind::Vector, {TypeKind::Any}, false, TypeKind::Vector,
                b_vector_append),

    // float::abs
    BuiltinFunc("abs", TypeKind::Float, {}, false, TypeKind::Float, b_float_abs),

    // float::floor
    BuiltinFunc("floor", TypeKind::Float, {}, false, TypeKind::Float, b_floor),

    // float::ceil
    BuiltinFunc("ceil", TypeKind::Float, {}, false, TypeKind::Float, b_ceil),

    // float::round
    BuiltinFunc("round", TypeKind::Float, {}, false, TypeKind::Float, b_round),

    // int::abs
    BuiltinFunc("abs", TypeKind::Int, {}, false, TypeKind::Int, b_abs),

    // int::pow
    BuiltinFunc("pow", TypeKind::Int, {TypeKind::Int}, false, TypeKind::Int, b_pow),

    // int::fib
    BuiltinFunc("fib", TypeKind::Int, {}, false, TypeKind::Int, b_fib),

    // (any)::to_string
    BuiltinFunc("to_string", TypeKind::Any, {}, false, TypeKind::String, b_to_string),
};

Obj BuiltinFunc::call(Evaluator& eval, Vec<Obj> const& args) const {
  return impl(eval, args);
}

string BuiltinFunc::to_string() const {
  string s;

  if (this->is_method)
    s += this->self_type.to_string() + "::";

  s += this->name + "(" +
       utils::join(", ", this->arg_types,
                   [](TypeInfo const& t) -> string {
                     return t.to_string();
                   }) +
       ")";

  return s;
}

size_t BuiltinFunc::find(Vec<BuiltinFunc const*>& out, string const& name) {
  for (auto&& bf : builtins) {
    if (bf.name == name)
      out.emplace_back(&bf);
  }

  return out.size();
}

BuiltinFunc::BuiltinFunc(string name, TypeInfo const& self_type, Vec<TypeInfo> arg_types,
                         bool is_variable_args, TypeInfo ret_type, Impl impl)
    : name(name),
      is_method(true),
      self_type(self_type),
      arg_types(arg_types),
      is_variable_args(is_variable_args),
      ret_type(ret_type),
      impl(impl) {
}

BuiltinFunc::BuiltinFunc(string name, Vec<TypeInfo> arg_types, bool is_variable_args,
                         TypeInfo ret_type, Impl impl)
    : name(name),
      is_method(false),
      arg_types(arg_types),
      is_variable_args(is_variable_args),
      ret_type(ret_type),
      impl(impl) {
}

} // namespace Builtins
