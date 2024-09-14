#include <iostream>
#include <sstream>

#include "Builtin.h"
#include "Parser.h"
#include "alert.h"

namespace metro::builtins {

static ObjPointer Print(ObjVector args) {
  std::stringstream ss;

  for (auto&& obj : args)
    ss << obj->ToString();

  auto str = ss.str();

  std::cout << str;

  return ObjNew<ObjPrimitive>((i64)str.length());
}

static ObjPointer Println(ObjVector args) {
  auto ret = ObjNew<ObjPrimitive>(
      Print(std::move(args))->As<ObjPrimitive>()->vi + 1);

  std::cout << std::endl;

  return ret;
}

static ObjPointer Import(ObjVector args) {
  auto path = args[0]->ToString();

  alertmsg(path);

  auto source = std::make_shared<SourceStorage>(path);

  source->Open();

  if (!source->IsOpen()) {
    alert;
    return ObjNew<ObjNone>();
  }

  Lexer lexer{*source};

  auto ast = parser::Parser(lexer.Lex()).Parse();

  return ObjNew<ObjModule>(source, ast);
}

// clang-format off
static const std::vector<Function> g_builtin_functions = {

  { "print",    Print,     0, true },
  { "println",  Println,   0, true },

  { "@import",  Import,    1 },

};
// clang-format on

ObjPointer Function::Call(ObjVector args) const {
  return this->func(args);
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

} // namespace metro::builtins
