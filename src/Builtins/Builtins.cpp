#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>

#include "Utils.h"

#include "Object.h"
#include "Token/Token.h"
#include "Node/Node.h"

#include "Builtins.h"
#include "Driver/Error.h"

#include "Sema/Symbol.h"

#define def_builtin_func(_Name)                                                          \
  static Obj b_##_Name([[maybe_unused]] Evaluator& eval, [[maybe_unused]] Node* node,    \
                       [[maybe_unused]] Vec<Obj> const& args)

#define ArgError(_arg_index, _msg)                                                       \
  Error(node->nd_callfunc_args[_arg_index], _msg, ErrorType::RunTime).crash()

namespace fire::Builtins {

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
  auto res = b_print(eval, node, args);

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
// string::insert(self, pos: int, value: string) -> string
//
def_builtin_func(string_insert) {
  ObjPtr<ObjStr> str = args[0]->as_str();

  i64 pos = args[1]->as_int()->val;

  ObjPtr<ObjStr> value = args[2]->as_str();

  if (pos < 0 || static_cast<size_t>(pos) > str->length())
    ArgError(1, "index out of range");

  return ObjStr::make(str->val.insert(static_cast<size_t>(pos), value->val));
}

//
// string::substr(self, pos: int) -> string
//
def_builtin_func(substr_1) {
  ObjPtr<ObjStr> str = args[0]->as_str();

  i64 start = args[1]->as_int()->val;

  if (start < 0 || static_cast<size_t>(start) > str->length())
    ArgError(1, "index out of range");

  return ObjStr::make(str->val.substr(static_cast<size_t>(start)));
}

//
// string::substr(self, pos: int, len: int) -> string
//
def_builtin_func(substr_2) {
  ObjPtr<ObjStr> str = args[0]->as_str();

  i64 start = args[1]->as_int()->val;
  i64 len = args[2]->as_int()->val;

  if (start < 0 || static_cast<size_t>(start) > str->length())
    ArgError(1, "index out of range");

  if (len < 0)
    ArgError(2, "length must be positive");

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
// vector::pop(self) -> any
//
def_builtin_func(vector_pop) {
  ObjPtr<ObjVector> vec = args[0]->as_vector();

  if (vec->list.empty())
    ArgError(0, "vector is empty");

  auto res = vec->list.back();

  vec->list.pop_back();

  return res;
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
static i64 fib_impl(i64 n) {
  if (n < 2)
    return 1;

  return fib_impl(n - 1) + fib_impl(n - 2);
}

def_builtin_func(fib) {
  return ObjInt::make(fib_impl(args[0]->as_int()->val));
}

//
// int::chr(self) -> char
//
def_builtin_func(int_chr) {
  return ObjChar::make(
      std::u16string(1, static_cast<char16_t>(args[0]->as_int()->val))[0]);
}

//
// char::ord(self) -> int
//
def_builtin_func(char_ord) {
  return ObjInt::make(static_cast<i64>(args[0]->as_char()->val));
}

//
// (any)::to_str(self) -> string
//
def_builtin_func(to_str) {
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

    // string::insert
    BuiltinFunc("insert", TypeKind::String, {TypeKind::Int, TypeKind::String}, false,
                TypeKind::String, b_string_insert),

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

    // vector::pop
    BuiltinFunc("pop", TypeKind::Vector, {}, false, TypeKind::Any, b_vector_pop),

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

    // int::chr
    BuiltinFunc("chr", TypeKind::Int, {}, false, TypeKind::Char, b_int_chr),

    // char::ord
    BuiltinFunc("ord", TypeKind::Char, {}, false, TypeKind::Int, b_char_ord),

    // (any)::to_str
    BuiltinFunc("to_str", TypeKind::Any, {}, false, TypeKind::String, b_to_str),
};

Obj BuiltinFunc::call(Evaluator& eval, Node* node, Vec<Obj> const& args) const {
  return impl(eval, node, args);
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

using namespace fire::sema;

static Vec<Symbol*> _func_symbols;
static Vec<Symbol*> _type_symbols;

Vec<Symbol*> const& Symbols::get_func_symbols() {
  return _func_symbols;
}

Vec<Symbol*> const& Symbols::get_type_symbols() {
  return _type_symbols;
}

size_t Symbols::find(Vec<Symbol*>& out, string const& name) {

  for (auto&& s : _func_symbols) {
    if (s->name == name)
      out.push_back(s);
  }

  for (auto&& s : _type_symbols) {
    if (s->name == name)
      out.push_back(s);
  }

  return out.size();
}

//
// make_sym_bfun:
//   Create a symbol for built-in function name.
//
static Symbol* make_sym_bfun(BuiltinFunc const& bfun) {
  auto sym = new Symbol(SY_BuiltinFunc);

  sym->name = bfun.name;
  sym->bfun = &bfun;

  return sym;
}

//
// make_sym_type:
//  Create a symbol for built-in type name.
//
static Symbol* make_sym_type(TypeKind tk, string const& name) {
  auto sym = new Symbol(SY_BuiltinType);

  sym->name = name;
  sym->tk = tk;

  return sym;
}

void initialize() {

  for (auto&& bf : builtins)
    _func_symbols.push_back(make_sym_bfun(bf));

  _type_symbols.push_back(make_sym_type(TypeKind::None, "none"));

  _type_symbols.push_back(make_sym_type(TypeKind::Int, "int"));
  _type_symbols.push_back(make_sym_type(TypeKind::Float, "float"));
  _type_symbols.push_back(make_sym_type(TypeKind::Bool, "bool"));
  _type_symbols.push_back(make_sym_type(TypeKind::Char, "char"));
  _type_symbols.push_back(make_sym_type(TypeKind::String, "string"));

  _type_symbols.push_back(make_sym_type(TypeKind::Vector, "vector"));
  _type_symbols.push_back(make_sym_type(TypeKind::Tuple, "tuple"));
  _type_symbols.push_back(make_sym_type(TypeKind::Dict, "dict"));
}

} // namespace fire::Builtins
