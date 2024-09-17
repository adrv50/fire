#include <iostream>
#include <sstream>

#include "Builtin.h"
#include "Parser.h"
#include "Evaluator.h"

#include "AST.h"
#include "Error.h"

#include "alert.h"

#define define_builtin_func(_Name_)                                  \
  ObjPointer _Name_(ASTPtr<AST::CallFunc> ast, ObjVector args)

#define expect_type(_Idx, _Type) _expect_type(ast, args, _Idx, _Type)

namespace fire::builtins {

void _expect_type(ASTPtr<AST::CallFunc> ast, ObjVector& args,
                  int index, TypeInfo const& type) {
  if (!args[index]->type.equals(type))
    Error(ast->args[index]->token,
          "expected '" + type.to_string() +
              "' type object at argument " + std::to_string(index) +
              ", but given '" + args[index]->type.to_string() +
              "'")();
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
  auto ret = ObjNew<ObjPrimitive>(
      Print(ast, std::move(args))->As<ObjPrimitive>()->vi + 1);

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

define_builtin_func(Import) {
  auto path = args[0]->ToString();

  auto source = std::make_shared<SourceStorage>(path);

  source->Open();

  if (!source->IsOpen()) {
    Error(ast->args[0]->token, "cannot open file '" + path + "'")();
  }

  Lexer lexer{*source};

  auto prg = parser::Parser(lexer.Lex()).Parse();

  eval::Evaluator ev{prg};

  auto ret = ObjNew<ObjModule>(source, prg);

  ret->name = path.substr(0, path.rfind('.'));

  if (auto slash = ret->name.rfind('/'); slash != std::string::npos) {
    ret->name = ret->name.substr(slash + 1);
  }

  for (auto&& x : prg->list) {
    switch (x->kind) {
    case ASTKind::Class:
      ret->types.emplace_back(
          ObjNew<ObjType>(ASTCast<AST::Class>(x)));
      break;

    case ASTKind::Enum:
      ret->types.emplace_back(ObjNew<ObjType>(ASTCast<AST::Enum>(x)));
      break;

    case ASTKind::Function:
      ret->functions.emplace_back(
          ObjNew<ObjCallable>(ASTCast<AST::Function>(x)));
      break;

    case ASTKind::Vardef: {
      auto y = ASTCast<AST::VarDef>(x);

      ret->variables[y->GetName()] = ev.evaluate(y->init);

      break;
    }
    }
  }

  return ret;
}

// clang-format off
static const std::vector<Function> g_builtin_functions = {

  { "print",    Print,     0, true },
  { "println",  Println,   0, true },

  { "open",     Open,      1 },

  { "@import",  Import,    1 },

};
// clang-format on

ObjPointer Function::Call(ASTPtr<AST::CallFunc> ast,
                          ObjVector args) const {
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

std::vector<builtins::Function> const& get_builtin_functions() {
  return g_builtin_functions;
}

} // namespace fire::builtins
