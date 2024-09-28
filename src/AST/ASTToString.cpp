#include "alert.h"
#include "Utils.h"
#include "AST.h"

namespace fire::AST {

int indent = 0;

static string get_indent() {
  return string((size_t)(indent * 2), ' ');
}

template <std::derived_from<AST::Base> T>
static string join(string const& s, std::vector<ASTPtr<T>> const& v) {
  return utils::join(s, v, ToString);
}

string ToString(ASTPointer ast) {
  if (!ast)
    return "(null)";

  switch (ast->kind) {
  case ASTKind::Identifier:
    if (auto x = ast->As<Identifier>(); !x->id_params.empty()) {
      return x->token.str + "@<" + utils::join(", ", x->id_params, ToString) + ">";
    }

  case ASTKind::Value:
  case ASTKind::Variable:
    return ast->token.str;

  case ASTKind::CallFunc: {
    auto x = ast->As<AST::CallFunc>();

    return ToString(x->callee) + "(" + join(", ", x->args) + ")";
  }

  case ASTKind::Block: {
    indent++;

    auto s = "{\n" + get_indent() +
             utils::join(get_indent() + ";\n", ast->As<Block>()->list, ToString);

    indent--;
    s += "\n" + get_indent() + "}";

    return s;
  }
  }

  todo_impl;
}

} // namespace fire::AST