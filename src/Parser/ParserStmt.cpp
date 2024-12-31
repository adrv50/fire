#include "alert.h"
#include "Error.h"
#include "Parser.h"

//
// stmt ::=
//   block | let | func |
//   (return expr? | break | continue) semi |
//   expr semi
//
Node* Parser::p_stmt() {
  //
  // block
  //
  if (auto node = this->p_block())
    return node;

  //
  // let
  //
  else if ((node = this->p_let()))
    return node;

  //
  // func
  //
  else if ((node = this->p_func()))
    return node;

  //
  // loop
  //
  else if ((node = this->p_loop()))
    return node;

  //
  // return
  //
  else if (this->eat(Kwd::Return)) {
    auto node = Node::new_node(ND_Return, this->cur);

    if (!this->eat_semi()) {
      node->nd_return_expr = this->p_expr();
      this->expect_semi();
    }

    return node;
  }

  //
  // break
  //
  else if (this->eat(Kwd::Break)) {
    auto node = Node::new_node(ND_Break, this->cur);
    this->expect_semi();
    return node;
  }

  //
  // continue
  //
  else if (this->eat(Kwd::Continue)) {
    auto node = Node::new_node(ND_Continue, this->cur);
    this->expect_semi();
    return node;
  }

  //
  // if
  //
  else if (this->eat(Kwd::If)) {
  }

  auto expr = this->p_expr();

  this->expect_semi();

  return expr;
}

//
// let ::=
//   let ident (":" type)? ("=" expr)? semi
//
Node* Parser::p_let() {

  if (this->eat(Kwd::Let)) {
    auto node = Node::new_node(ND_Let, this->cur);

    node->nd_let_name = this->expect_ident();

    if (this->eat(Punct::Colon)) {
      node->nd_let_type = this->p_expect_type();
    }

    if (this->eat(Op::Assign)) {
      node->nd_let_init = this->p_expr();
    }

    this->expect_semi();

    return node;
  }

  return nullptr;
}

//
// loop ::=
//   loop block
//
Node* Parser::p_loop() {
  if (this->eat(Kwd::Loop)) {
    auto node = Node::new_node(ND_Loop, this->cur);

    node->nd_loop_body = this->p_block();

    return node;
  }

  return nullptr;
}

//
// block ::=
//   "{" stmt* "}"
//
Node* Parser::p_block(bool expected) {
  if ((expected && this->expect(Punct::BlockBraceOpen)) ||
      this->eat(Punct::BlockBraceOpen)) {
    auto node = Node::new_node(ND_Block, this->cur);

    while (!this->match(Punct::BlockBraceClose))
      node->append(this->p_stmt());

    this->expect(Punct::BlockBraceClose);

    return node;
  }

  return nullptr;
}
