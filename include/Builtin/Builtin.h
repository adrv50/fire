#pragma once

#include <string>
#include "Parser/AST/AST.h"

namespace metro {
namespace builtin {

struct Function {
  using FuncPointer = ObjPointer (*)(ObjVector);

  std::string name;
  std::vector<TypeInfo> arg_types;
  TypeInfo result_type;
  bool is_variable_args;
  FuncPointer func;

  ObjPointer Call(ObjVector args) const;
};

struct Class {
  std::string name;
  std::vector<TypeInfo> member_var_types; // メンバ変数
  std::vector<Function> member_functions;

  static ObjPointer NewInstance();
};

struct Module {
  std::string name;
  std::vector<ASTPtr<AST::Class>> classes;
  std::vector<Function> functions;
};

} // namespace builtin

struct MetroBuiltins {
  std::vector<builtin::Function> functions;
  std::vector<builtin::Class> classes;
  std::vector<builtin::Module> modules;
};

MetroBuiltins const& get_metro_builtins();

} // namespace metro