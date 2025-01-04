#include "alert.h"
#include "Object.h"
#include "Token.h"
#include "Node.h"
#include "Error.h"
#include "Parser.h"

// -----------------
//  Parser::p_expect_type
// ----------------------------------
Node* Parser::p_expect_type() {
  auto tok = this->cur;

  auto node = Node::new_node(ND_TypeName, this->expect_ident());

  node->first_tok = tok;

  if (this->eat_template_args_open()) {
    do {
      node->append(this->p_expect_type());
    } while (this->eat(Punct::Comma));

    this->expect_template_args_close();
  }

  node->last_tok = this->cur->prev;

  return node;
}

// -----------------
//  Parser::p_expect_identifier
// ----------------------------------
Node* Parser::p_expect_identifier(bool allow_qualifier) {
  auto tok = this->cur;

  auto node = Node::new_node(ND_Identifier, this->expect_ident());

  node->first_tok = node->last_tok = tok;

  if (allow_qualifier)
    this->p_parse_id_qualifier(node);

  return node;
}

// -----------------
//  Parser::p_parse_id_qualifier
// ----------------------------------
void Parser::p_parse_id_qualifier(Node* nd) {
  auto save1 = this->cur;
  auto save2 = this->ate;

  try {
    if (this->eat_template_args_open()) { // eat '<'
      do {
        nd->append(this->p_scope_resol());
      } while (this->eat(Punct::Comma));

      this->expect_template_args_close();

      nd->last_tok = this->cur->prev;
    }
  }

  catch (Error const& e) { // --> compare operator '<'
    this->cur = save1;
    this->ate = save2;
  }
}

// -----------------
//  Parser::p_expect_pair_name_and_type
//
//    => "a: T"
// ----------------------------------
Node* Parser::p_expect_pair_name_and_type() {
  auto tok = this->cur;

  auto node = Node::new_node(ND_PairNameAndType, this->expect_ident());

  node->first_tok = node->nd_nametype_pair_name = tok;

  this->expect_colon();

  node->nd_nametype_pair_type = this->p_expect_type();

  node->last_tok = this->cur->prev;

  return node;
}

// -----------------
//  Parser::p_expect_initializer_list
//
//    => "{a: 1, b: 2, ...}"
// ----------------------------------
Node* Parser::p_expect_initializer_list() {
  auto tok = this->cur;

  auto node = Node::new_node(ND_InitializerList, tok);

  node->first_tok = tok;

  this->expect_brace_open();

  do {
    node->append(this->p_expect_pair_name_and_type());
  } while (this->eat_comma());

  this->expect_brace_close();

  node->last_tok = this->cur->prev;

  return node;
}
