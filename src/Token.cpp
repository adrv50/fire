#include "Token.h"

namespace fire {

Token const* Token::get_prev(int step) {
  if (this->_index - step < 0)
    return nullptr;

  return &this->sourceloc.ref->token_list[this->_index - step];
}

Token const* Token::get_next(int step) {
  auto const& list = this->sourceloc.ref->token_list;

  if (this->_index + step >= (i64)list.size())
    return nullptr;

  return &list[this->_index + step];
}

} // namespace fire