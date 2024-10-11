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

string tpn2str_fn(AST::Templatable::ParameterName const& P) {

  string s = string(P.token.str);

  if (P.params.size() >= 1) {
    s += "<" + utils::join(", ", P.params, tpn2str_fn) + ">";
  }

  return s;
}

string ToString(ASTPointer ast) {
  if (!ast)
    return "(null)";

  switch (ast->_constructed_as) {
  case ASTKind::Function: {
    auto x = ast->As<Function>();

    string s;

    if (x->is_virtualized)
      s = "virtual ";

    s += "fn " + x->GetName();

    if (x->is_templated) {
      s += " <" + utils::join(", ", x->template_param_names, tpn2str_fn) + ">";
    }

    s += " (" + join(", ", x->arguments) + ") ";

    if (x->return_type) {
      s += "-> " + ToString(x->return_type) + " ";
    }

    if (x->is_overrided) {
      s += "override ";
    }

    return s + ToString(x->block);
  }

  case ASTKind::Argument: {
    auto x = ast->As<Argument>();

    return x->name.str + ": " + ToString(x->type);
  }

  case ASTKind::TypeName: {
    auto x = ast->As<TypeName>();

    string s = string(x->GetName());

    if (!x->type_params.empty()) {
      s += "<" + utils::join<ASTPtr<TypeName>>(", ", x->type_params, ToString) + ">";
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
    return string(ast->token.str);

  case ASTKind::Array: {
    auto x = ast->As<Array>();

    return "[" + join(", ", x->elements) + "]";
  }

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

    auto s = "{\n" + get_indent() + join(";\n" + get_indent(), ast->As<Block>()->list);

    indent--;
    s += "\n" + get_indent() + "}";

    return s;
  }

  case ASTKind::While: {
    auto d = ast->as_stmt()->data_while;

    return "while " + ToString(d->cond) + " " + ToString(d->block);
  }

  case ASTKind::If: {
    auto d = ast->as_stmt()->data_if;

    auto s = "if " + ToString(d->cond) + " " + ToString(d->if_true);

    if (d->if_false)
      s += "else " + ToString(d->if_false);

    return s;
  }

  case ASTKind::Return: {
    auto ex = ast->as_stmt()->expr;

    if (ex)
      return "return " + ToString(ex) + ";";

    return "return;";
  }

  case ASTKind::Vardef: {
    auto x = ASTCast<AST::VarDef>(ast);

    auto s = "let " + x->GetName();

    if (x->type)
      s += " : " + ToString(x->type);

    if (x->init)
      s += " = " + ToString(x->init);

    return s + ";";
  }

  case ASTKind::MemberAccess: {
    auto x = ast->as_expr();

    return ToString(x->lhs) + "." + ToString(x->rhs);
  }

  case ASTKind::IndexRef: {
    auto x = ast->as_expr();

    return ToString(x->lhs) + "[" + ToString(x->rhs) + "]";
  }
  }

  alertexpr(static_cast<int>(ast->kind));
  assert(ast->is_expr);

  auto x = ast->as_expr();

  string ops = x->op.str;

  switch (x->kind) {
  case ASTKind::Bigger:
    ops = ">";
    break;

  case ASTKind::BiggerOrEqual:
    ops = ">=";
    break;
  }

  return ToString(x->lhs) + " " + ops + " " + ToString(x->rhs);
}

} // namespace fire::AST