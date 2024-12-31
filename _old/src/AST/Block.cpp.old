#include "AST.h"
#include "alert.h"

namespace fire::AST {

ASTPtr<Block> Block::New(Token tok, ASTVector list) {
  return ASTNew<Block>(tok, std::move(list));
}

ASTPointer Block::Clone() const {
  auto x = New(this->token);

  for (ASTPointer const& a : this->list)
    x->list.emplace_back(a->Clone());

  x->ScopeCtxPtr = this->ScopeCtxPtr;

  return x;
}

Block::Block(Token tok, ASTVector list)
    : Base(ASTKind::Block, tok),
      list(std::move(list)) {
}

} // namespace fire::AST
