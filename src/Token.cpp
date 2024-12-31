#include "Token.h"

bool Token::is(TokenKind k) const {
  return this->kind == k;
}

bool Token::is_kwd(TokenKwdKind k) const {
  return this->kwd == k;
}

bool Token::is_op(TokenOperatorKind k) const {
  return this->op == k;
}

bool Token::is_punct(TokenPunctKind k) const {
  return this->punct == k;
}

bool Token::is_semi() const {
  return is(TokenKind::Semi);
}

Token* Token::make(TokenKind kind, SourceStorage const* SS, Token* prev,
                   string const& str, size_t pos, TokenKwdKind kwd, TokenOperatorKind op,
                   TokenPunctKind punct) {
  auto tok = new Token(kind, SS, prev, str, pos, kwd, op, punct);

  if (prev)
    prev->next = tok;

  return tok;
}

Token* Token::make(TokenKind kind) {
  return make(kind, nullptr, nullptr, "", 0);
}

Token* Token::set_kwd(TokenKwdKind k) {
  this->kwd = k;
  return this;
}

Token* Token::set_op(TokenOperatorKind k) {
  this->op = k;
  return this;
}

Token* Token::set_punct(TokenPunctKind k) {
  this->punct = k;
  return this;
}

Token* Token::clone() const {
  auto cloned = Token::make(this->kind, &this->ref->get_ss(), this->prev, this->str,
                            this->ref->pos, this->kwd, this->op, this->punct);

  cloned->next = this->next;
  cloned->prev = this->prev;

  return cloned;
}

Token::Token(TokenKind kind, SourceStorage const* SS, Token* prev, string const& str,
             size_t pos, TokenKwdKind kwd, TokenOperatorKind op, TokenPunctKind punct)
    : kind(kind),
      kwd(kwd),
      op(op),
      punct(punct),
      prev(prev),
      next(nullptr),
      str(str),
      ref(SS ? SS->make_ref(this, pos, str.length()) : nullptr),
      literal_data({0}),
      v_str() {
}

Token::Token(TokenKind kind)
    : Token(kind, nullptr, nullptr, "", 0, TokenKwdKind::None, TokenOperatorKind::None,
            TokenPunctKind::None) {
}