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

  switch (ast->_constructed_as) {
  case ASTKind::TypeName: {
    auto x = ast->As<TypeName>();

    string s = x->GetName();

    if (!x->type_params.empty()) {
      s += "@<" + utils::join<ASTPtr<TypeName>>(", ", x->type_params, ToString) + ">";
    }

    if (x->is_const)
      s += " const";

    return s;
  }

  case ASTKind::Identifier:
    if (auto x = ast->As<Identifier>(); !x->id_params.empty()) {
      return x->token.str + "@<" + join(", ", x->id_params) + ">";
    }

  case ASTKind::Value:
    return ast->token.str;

  case ASTKind::ScopeResol: {
    auto x = ast->As<ScopeResol>();

    auto s = ToString(x->first);

    for (auto&& id : x->idlist)
      s += "::" + ToString(id);

    return s;
  }

  case ASTKind::CallFunc: {
    auto x = ast->As<AST::CallFunc>();

    return ToString(x->callee) + "(" + join(", ", x->args) + ")";
  }

  case ASTKind::Block: {
    indent++;

    auto s = "{\n" + get_indent() + join(get_indent() + ";\n", ast->As<Block>()->list);

    indent--;
    s += "\n" + get_indent() + "}";

    return s;
  }
  }

  assert(ast->is_expr);

  auto x = ast->as_expr();

  return ToString(x->lhs) + x->op.str + ToString(x->rhs);
}

} // namespace fire::AST