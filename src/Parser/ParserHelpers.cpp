#include "alert.h"
#include "Object.h"
#include "Token.h"
#include "Node.h"
#include "Error.h"
#include "Parser.h"

namespace fire {

// -----------------
//  Parser::p_expect_type
// ----------------------------------
Node* Parser::p_expect_type() {
  Token* tok = this->cur;

  Node* type = this->p_expect_type_part();

  while (this->eat(Punct::ScopeResol))
    type->append(this->p_expect_type_part());

  type->nd_type_is_mut = this->eat(Kwd::Mut);
  type->nd_type_is_ref = this->eat(Kwd::Ref);

  type->first_tok = tok;
  type->last_tok = this->cur->prev;

  return type;
}

Node* Parser::p_expect_type_part() {
  auto tok = this->cur;

  auto node = Node::new_node(ND_TypeName, tok);

  node->first_tok = tok;

  node->nd_type_id = this->p_expect_identifier();

  if (!node->nd_type_id->is_id_or_sr())
    Error(tok, "expected identifier").crash();

  if (this->eat_tp_args_open()) {
    node->nd_type_tp_args = Node::new_node(ND_TemplateArguments, this->cur->prev);

    do {
      node->nd_type_tp_args->append(this->p_expect_type());
    } while (this->eat(Punct::Comma));

    this->expect_tp_args_close();
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
    if (this->eat_tp_args_open()) { // eat '<'
      do {
        nd->append(this->p_scope_resol());
      } while (this->eat(Punct::Comma));

      this->expect_tp_args_close();

      nd->last_tok = this->cur->prev;
    }
  }

  catch (Error const& e) { // --> compare operator '<'
    this->cur = save1;
    this->ate = save2;

    nd->nd_id_tp_args.clear();
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

Node* Parser::eat_expr() {
  return EAT_EXPR(p_expr);
}

Node* Parser::eat_expr(std::function<Node*()> fn) {
  auto _tok = this->cur;

  try {
    return fn();
  }
  catch (Error const&) {
    this->cur = _tok;
  }

  return nullptr;
}

Node* Parser::expect_pr_expr(std::function<Node*()> fn) {
  auto _tok = this->cur->prev;

  if (auto x = this->eat_expr(fn))
    return x;

  Error(_tok, "expected primary-expression after this token").crash();
}

Node* Parser::eat_template_parameter_list() {
  auto tok = this->cur;

  if (!this->eat(Punct::AngleBraceOpen))
    return nullptr;

  auto tplist = Node::new_node(ND_TemplateParameterList, tok, nullptr);

  do {
    tplist->append(this->p_expect_identifier(false));
  } while (this->eat_comma());

  this->expect(Punct::AngleBraceClose);

  return tplist;
}

//
// [Concept(T, U), ...]
//
Node* Parser::eat_concept_tags_list() {
  if (!this->eat(Punct::ArrayBraceOpen))
    return nullptr;

  auto tags = Node::new_node(ND_ConceptTagsList, this->cur->prev, nullptr);

  tags->first_tok = this->cur->prev;

  do {
    tags->append(this->expect_concept_tag());
  } while (this->eat_comma());

  tags->last_tok = this->cur;
  this->expect(Punct::ArrayBraceClose);

  return tags;
}

//
// 1. Concept(T)
// 2. (ConceptA(T) or ConceptB(T))
//
Node* Parser::expect_concept_tag() {
  auto tok = this->cur;

  if (this->eat_brace_open()) {
    auto nd = Node::new_node(ND_ConceptTagMulti, tok, nullptr);

    nd->append(expect_concept_tag());
    this->expect(Kwd::Or);

    do {
      nd->append(expect_concept_tag());
    } while (this->eat(Kwd::Or));

    this->expect_brace_close();

    return nd;
  }

  auto nd = Node::new_node(ND_ConceptTag, this->expect_ident(), nullptr);

  this->expect_tp_args_open();

  do {
    nd->append(this->p_expect_identifier(false));
  } while (this->eat_comma());

  this->expect_tp_args_close();

  return nd;
}

} // namespace fire