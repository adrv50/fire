#include "AST.h"

namespace fire::AST {

ASTPtr<Array> Array::New(Token tok) {
  return ASTNew<Array>(tok);
}

ASTPointer Array::Clone() const {
  auto x = New(this->token);

  for (ASTPointer const& e : this->elements)
    x->elements.emplace_back(e->Clone());

  return x;
}

Array::Array(Token tok)
    : Base(ASTKind::Array, tok) {
}

} // namespace fire::AST