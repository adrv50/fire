#include "Sema/Sema.h"

namespace metro::sema {

Sema::Sema(AST::Program root) : _root(root) {
  for (auto&& ast : root.list) {
    switch (ast->kind) {
    case ASTKind::Enum:
      this->enums.emplace_back(
          std::static_pointer_cast<AST::Enum>(ast));
      break;

    case ASTKind::Class:
      this->classes.emplace_back(
          std::static_pointer_cast<AST::Class>(ast));
      break;

    case ASTKind::Function:
      this->functions.emplace_back(
          std::static_pointer_cast<AST::Function>(ast));
      break;
    }
  }
}

struct IDSearchResult {
  ASTPointer ast;
};

ASTPointer FindNameInAST(ASTPointer ast, std::string_view name) {

  if (ast->is_named && ast->As<AST::Named>()->GetName() == name)
    return ast;

  switch (ast->kind) {
  case ASTKind::Enum:
    for (auto&& etor : ast->As<AST::Enum>()->enumerators) {
      if (etor->name.str == name)
        return etor;
    }

    break;

  case ASTKind::Class: {
    auto _class = ast->As<AST::Class>();

    for (auto&& var : _class->mb_variables) {
      if (var->name.str == name)
        return var;
    }

    for (auto&& fun : _class->mb_functions) {
      if (fun->name.str == name)
        return fun;
    }

    break;
  }

  case ASTKind::Namespace: {
    for (auto&& x : ast->As<AST::Namespace>()->list) {
      if (auto p = FindNameInAST(x, name); p)
        return p;
    }

    break;
  }
  }

  return nullptr;
}

IDSearchResult FindIdentifier() {
}

TypeInfo Sema::EvalExprType(ASTPointer ast) {
  switch (ast->kind) { case ASTKind::Value: }

  return TypeKind::None;
}

} // namespace metro::sema