#include "Driver/Error.h"
#include "Sema/Sema.h"
#include "Debug/Debug.h"

namespace fire::sema {

TypeInfo ExprEval::expect_enumerator_type(Node* node) {
  auto type = this->eval(node);

  if (!type.is_enumerator()) {
    Error(node,
          "expected enumerator type expression, but found '" + type.to_string() + "'")
        .crash();
  }

  return type;
}

TypeInfo ExprEval::expect_lvalue(Node* node) {
  auto ti = this->eval(node);

  if (!this->is_lvalue(node))
    Error(node, "expected lvalue expression").crash();

  return ti;
}

TypeInfo ExprEval::expect_lvalue(Node* node, TypeInfo const& _expect) {
  return this->expect(node, this->expect_lvalue(node));
}

bool ExprEval::is_lvalue(Node* node) {

  debug assert(node != nullptr);

  switch (node->kind) {
    case ND_Identifier:
    case ND_ScopeResol:
      try {
        this->eval(node);
        return node->kind == ND_Variable;
      }
      catch (Error const& e) {
        return false;
      }
      break;

    case ND_Variable:
      return true;

    case ND_Subscript:
    case ND_MemberAccess:
      return this->is_lvalue(node->nd_lhs);
  }

  return false;
}

} // namespace fire::sema