#include <iostream>
#include <sstream>

#include "Builtin.h"
#include "Parser.h"
#include "Evaluator.h"

#include "AST.h"
#include "Error.h"

#include "alert.h"

namespace fire::builtins {

static ObjPointer Print(ASTPtr<AST::CallFunc> ast, ObjVector args) {
  std::stringstream ss;

  for (auto&& obj : args)
    ss << obj->ToString();

  auto str = ss.str();

  std::cout << str;

  return ObjNew<ObjPrimitive>((i64)str.length());
}

static ObjPointer Println(ASTPtr<AST::CallFunc> ast, ObjVector args) {
  auto ret = ObjNew<ObjPrimitive>(
      Print(ast, std::move(args))->As<ObjPrimitive>()->vi + 1);

  std::cout << std::endl;

  return ret;
}

ObjPointer Import(ASTPtr<AST::CallFunc> ast, ObjVector args) {
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
