#include "alert.h"
#include "Token.h"
#include "Node.h"
#include "Error.h"
#include "Parser.h"

bool Parser::check() {
  return !this->cur->is(TokenKind::End);
}

Token* Parser::next() {
  auto tok = this->cur;

  this->cur = this->cur->next;

  return tok;
}

Token* Parser::insert(Token* tok) {
  return this->cur->insert(tok);
}

bool Parser::match(TokenKind k) {
  return this->cur->is(k);
}

bool Parser::match(TokenPunctKind k) {
  return this->cur->is_punct(k);
}

bool Parser::match(TokenOperatorKind k) {
  return this->cur->is_op(k);
}

bool Parser::match(TokenKwdKind k) {
  return this->cur->is_kwd(k);
}

bool Parser::eat(TokenKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

bool Parser::eat(TokenPunctKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

bool Parser::eat(TokenOperatorKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

bool Parser::eat(TokenKwdKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

Token* Parser::expect(TokenKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected " + Token::kind_to_str(k) + " but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Token* Parser::expect(TokenPunctKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected '" + Token::punct_to_str(k) + "' but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Token* Parser::expect(TokenOperatorKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected '" + Token::op_to_str(k) + "' but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Token* Parser::expect(TokenKwdKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected '" + Token::kwd_to_str(k) + "' but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Node* Parser::new_zero() {
  return Node::new_value(nullptr, ObjInt::make(0));
}

//
// node_assign_with_op:
//   wrapper for create a new node of assign-expr with operator
//
// A op # "=" B
//   --> A = (A op B)
//
Node* Parser::new_assign_with_op(NodeKind kind, Token* tok, Node* lhs, Node* rhs) {
  return Node::new_node(ND_Assign, tok, lhs, Node::new_node(kind, tok, lhs, rhs));
}

bool Parser::eat_ident() {
  return this->eat(TokenKind::Identifier);
}

Token* Parser::expect_ident() {
  return this->expect(TokenKind::Identifier);
}

bool Parser::eat_semi() {
  return this->eat(TokenKind::Semi);
}

Token* Parser::expect_semi() {
  return this->expect(TokenKind::Semi);
}

bool Parser::eat_template_args_open() {
  return this->eat(Punct::AngleBraceOpen);
}

bool Parser::eat_template_args_close() {
  if (this->match(Op::RShift)) {
    this->cur->set_punct(Punct::AngleBraceClose);

    this->insert(Token::make(TokenKind::Punctuator)->set_punct(Punct::AngleBraceClose));
  }

  return this->eat(Punct::AngleBraceClose);
}

Token* Parser::expect_template_args_open() {
  if (!this->eat_template_args_open())
    Error(this->cur, "expected '<' but found '" + this->cur->str + "'").crash();

  return this->cur->prev;
}

Token* Parser::expect_template_args_close() {
  if (!this->eat_template_args_close())
    Error(this->cur, "expected '>' but found '" + this->cur->str + "'").crash();

  return this->cur->prev;
}

Node* Parser::p_expect_type() {
  auto node = Node::new_node(ND_TypeName, this->expect_ident());

  if (this->eat_template_args_open()) {
    do {
      node->append(this->p_expect_type());
    } while (this->eat(Punct::Comma));

    this->expect_template_args_close();
  }

  return node;
}

Node* Parser::p_expect_identifier(bool allow_qualifier) {
  auto node = Node::new_node(ND_Identifier, this->expect_ident());

  if (allow_qualifier)
    this->p_parse_id_qualifier(node);

  return node;
}

void Parser::p_parse_id_qualifier(Node* nd) {
  auto save1 = this->cur;
  auto save2 = this->ate;

  try {
    if (this->eat_template_args_open()) { // eat '<'
      do {
        nd->append(this->p_expect_type());
      } while (this->eat(Punct::Comma));

      this->expect_template_args_close();
    }
  }

  catch (Error const& e) { // --> compare operator '<'
    this->cur = save1;
    this->ate = save2;
  }
}
