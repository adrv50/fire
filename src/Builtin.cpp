#include <iostream>
#include <sstream>

#include "alert.h"
#include "AST.h"
#include "Builtin.h"
#include "Object.h"
#include "Error.h"

#define define_builtin_func(_Name_)                                                      \
  ObjPointer _Name_([[maybe_unused]] ASTPtr<AST::CallFunc> ast,                          \
                    [[maybe_unused]] ObjVector args)

#define expect_type(_Idx, _Type) _expect_type(ast, args, _Idx, _Type)

namespace fire::builtins {

void _expect_type(ASTPtr<AST::CallFunc> ast, ObjVector& args, int index,
                  TypeInfo const& type) {
  if (!args[index]->type.equals(type))
    Error(ast->args[index]->token, "expected '" + type.to_string() +
                                       "' type object at argument " +
                                       std::to_string(index) + ", but given '" +
                                       args[index]->type.to_string() + "'")();
}

define_builtin_func(Print) {
  std::stringstream ss;

  for (auto&& obj : args)
    ss << obj->ToString();

  auto str = ss.str();

  std::cout << str;

  return ObjNew<ObjPrimitive>((i64)str.length());
}

define_builtin_func(Println) {
  auto ret =
      ObjNew<ObjPrimitive>(Print(ast, std::move(args))->As<ObjPrimitive>()->vi + 1);

  std::cout << std::endl;

  return ret;
}

define_builtin_func(Open) {
  expect_type(0, TypeKind::String);

  auto path = args[0]->ToString();

  std::ifstream ifs{path};

  if (!ifs.is_open())
    return ObjNew<ObjNone>();

  std::string data;

  for (std::string line; std::getline(ifs, line); data += line + '\n')
    ;

  return ObjNew<ObjString>(data);
}

define_builtin_func(Substr) {
  auto str = args[0]->As<ObjString>();
  auto pos = args[1]->As<ObjPrimitive>()->vi;

  if (pos < 0 || pos >= (i64)str->Length())
    Error(ast->args[1], "out of range")();

  return str->SubString(pos);
}

define_builtin_func(Substr2) {
  auto str = args[0]->As<ObjString>();
  auto pos = args[1]->As<ObjPrimitive>()->vi;
  auto len = args[2]->As<ObjPrimitive>()->vi;

  if (pos < 0 || pos >= (i64)str->Length())
    Error(ast->args[1], "out of range")();

  if (pos + len >= (i64)str->Length())
    Error(ast->args[2], "out of range")();

  return str->SubString(pos, len);
}

define_builtin_func(Length) {
  auto content = args[0];

  if (content->is_string() || content->is_vector()) {
    return ObjNew<ObjPrimitive>((i64)content->As<ObjString>()->list.size());
  }

  todo_impl;
}

define_builtin_func(ToString) {
  return ObjNew<ObjString>(args[0]->ToString());
}

// clang-format off
static const std::vector<Function> g_builtin_functions = {

  { "print",    Print,     TypeKind::Int, { }, true },
  { "println",  Println,   TypeKind::Int, { }, true },

  { "open",     Open,      TypeKind::String, { TypeKind::String }, },


};

static const vector<std::pair<TypeInfo, Function>>
g_builtin_member_functions = {
  { // substr(index)
    TypeKind::String, { "substr", Substr, TypeKind::String, { TypeKind::Int } }
  },

  { // substr(index, len)
    TypeKind::String, { "substr", Substr2, TypeKind::String, { TypeKind::Int, TypeKind::Int } }
  },

  { TypeKind::String, { "length", Length, TypeKind::Int, { }, } },
  { TypeKind::Vector, { "length", Length, TypeKind::Int, { }, } },
  
  { TypeKind::Unknown, { "to_string", ToString, TypeKind::String, { }, } },
  
  // { "length",   Length,    TypeKind::Int, { {TypeKind::Vector, {TypeKind::Unknown}} }, },

};

// clang-format on

#define make_builtin_member                                                              \
  []([[maybe_unused]] ASTPointer self_ast, [[maybe_unused]] ObjPointer self) -> ObjPointer

// clang-format off
static const vector<MemberVariable>
g_builtin_member_variables = {

{ "abs", TypeKind::Int, TypeKind::Int,
  make_builtin_member{
    i64 val = self->get_vi();

    if (val < 0)
      val = -val;

    return ObjNew<ObjPrimitive>(val);
  }
},

};
// clang-format on

ObjPointer Function::Call(ASTPtr<AST::CallFunc> ast, ObjVector args) const {
  return this->func(ast, std::move(args));
}

Function const* find_builtin_func(std::string const& name) {
  auto const& funcs = get_builtin_functions();

  for (auto&& f : funcs) {
    if (f.name == name)
      return &f;
  }

  return nullptr;
}

vector<builtins::Function> const& get_builtin_functions() {
  return g_builtin_functions;
}

vector<std::pair<TypeInfo, Function>> const& get_builtin_member_functions() {
  return g_builtin_member_functions;
}

vector<MemberVariable> const& get_builtin_member_variables() {
  return g_builtin_member_variables;
}

} // namespace fire::builtins
