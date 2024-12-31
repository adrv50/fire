#include "alert.h"
#include "Error.h"
#include "Parser.h"

//
// program ::=
//   root*
//
Node* Parser::parse() {
  auto node = Node::new_node(ND_Program, this->cur);

  while (this->check())
    node->append(this->p_root());

  return node;
}

//
// root ::=
//   let | func
//
Node* Parser::p_root() {
  if (auto node = this->p_let(); node)
    return node;

  else if ((node = this->p_func()))
    return node;

  else if (this->in_repl)
    return this->p_stmt();

  Error(this->cur, "expected function or variable declaration").crash();
}

//
// func ::=
//   func ident "(" func_args ("," func_args)* ")" ("->" type)? block
//
Node* Parser::p_func() {
  if (this->eat(Kwd::Func)) {
    auto node = Node::new_node(ND_Function, this->cur);

    node->nd_func_name = this->expect_ident();

    this->expect(Punct::BraceOpen);

    if (!this->eat(Punct::BraceClose)) {
      do {
        node->append(this->p_func_arg());
      } while (this->eat(Punct::Comma));

      this->expect(Punct::BraceClose);
    }

    if (this->eat(Punct::ResultTypeSpecifier)) {
      node->nd_func_result_type = this->p_expect_type();
    }

    node->nd_func_body = this->p_block(true);

    return node;
  }

  return nullptr;
}

//
// func_arg ::=
//   ident ":" type
//
Node* Parser::p_func_arg() {
  auto node = Node::new_node(ND_FunctionArg, this->cur);

  node->nd_func_arg_name = this->expect_ident();

  this->expect(Punct::Colon);

  node->nd_func_arg_type = this->p_expect_type();

  return node;
}
