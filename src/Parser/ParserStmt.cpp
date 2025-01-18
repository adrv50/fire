#include "alert.h"
#include "Token.h"
#include "Node.h"
#include "Error.h"
#include "Parser.h"

namespace fire {

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
  // loop-statements
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

    auto node = Node::new_node(ND_If, this->cur);

    node->nd_if_cond = this->p_expr();

    (node->nd_if_then = this->p_block())->nd_block_parent = node;

    if (this->eat(Kwd::Else)) {
      (node->nd_if_else = this->p_block())->nd_block_parent = node;
    }

    return node;
  }

  //
  // switch
  else if (this->eat(Kwd::Switch)) {
    auto node = Node::new_node(ND_Switch, this->cur);

    node->nd_switch_cond = this->p_expr();

    this->expect_block_open();

    Token* default_tok = nullptr;

    while (!this->match(Punct::BlockBraceClose)) {
      if (this->eat(Kwd::Default)) {
        if (default_tok)
          Error(default_tok, "redefinition of default case").crash();

        default_tok = this->cur;

        (node->nd_switch_default_case = this->p_block())->nd_block_parent = node;

        continue;
      }

      node->append(this->p_expect_switch_case());
    }

    this->expect_block_close();

    return node;
  }

  //
  // match
  //
  else if (this->eat(Kwd::Match)) {
    auto node = Node::new_node(ND_Match, this->cur);

    node->nd_match_cond = this->p_getexpr_rm_block();

    this->expect_block_open();

    if (this->match(Punct::BlockBraceClose)) {
      Error(node->tok->prev, "empty match statement is not valid").crash();
    }

    while (!this->match(Punct::BlockBraceClose)) {
      auto match_case = Node::new_node(ND_MatchCase, this->cur);

      match_case->nd_match_case_cond = this->p_expr();

      this->expect(Punct::CaseMatch);

      (match_case->nd_match_case_body = this->p_block())->nd_block_parent = match_case;

      node->append(match_case);

      if (this->eat(Punct::Comma))
        continue;

      if (this->match(Punct::BlockBraceClose))
        break;

      Error(this->cur, "expected ',' or '}'").crash();
    }

    this->expect_block_close();

    return node;
  }

  auto expr = this->p_expr();

  this->expect_semi();

  return expr;
}

Node* Parser::p_expect_switch_case() {
  auto node = Node::new_node(ND_SwitchCase, this->cur);

  this->expect(Kwd::Case);

  (node->nd_switch_case_cond = this->p_expr())->nd_block_parent = node;
  (node->nd_switch_case_body = this->p_block())->nd_block_parent = node;

  return node;
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
// loop-statements
//
Node* Parser::p_loop() {
  auto tok = this->cur;

  //
  // loop
  //
  if (this->eat(Kwd::Loop)) {
    auto node = Node::new_node(ND_Loop, this->cur);

    (node->nd_loop_body = this->p_block())->nd_block_parent = node;

    return node;
  }

  //
  // while
  //
  else if (this->eat(Kwd::While)) {
    auto node = Node::new_node(ND_While, this->cur);

    node->nd_while_cond = this->p_expr();

    (node->nd_while_body = this->p_block())->nd_block_parent = node;

    return node;
  }

  //
  // for
  //
  else if (this->eat(Kwd::For)) {
    auto node = Node::new_node(ND_For, this->cur);

    if (this->eat_semi()) {
      goto _end_first_parse;
    }

    // parse first expr
    {
      if (this->match(Kwd::Let)) {
        node->nd_for_init = this->p_let();
        goto _end_first_parse;
      }

      auto expr = this->p_getexpr_rm_block();

      //
      // for a ... b
      if (expr->is(ND_Range)) {
        node->kind = ND_ForRange;

        node->nd_forrange_range = expr;

        (node->nd_forrange_body = this->p_block())->nd_block_parent = node;

        return node;
      }

      //
      // for a in b
      if (expr->is(ND_In)) {
        node->kind = ND_ForEach;

        node->nd_foreach_iter = expr->nd_lhs;
        node->nd_foreach_content = expr->nd_rhs;

        (node->nd_foreach_body = this->p_block())->nd_block_parent = node;

        return node;
      }

      if (this->eat_semi()) {
        goto _end_first_parse;
      }

      if (this->eat(Punct::BlockBraceOpen)) {
        // parse as for-range without start value
        alertmsg("for-range without start value");
        todo_impl;
        // return
      }

      node->nd_for_init = expr;

      this->expect_semi();
    }
  _end_first_parse:;

    {
      Node* second = this->p_expr();

      //
      // foreach a; x in y; b; c
      if (second->is(ND_In)) {
        auto init = node->nd_for_init;

        node = Node::new_node(ND_ForEach, tok, nullptr);

        node->nd_foreach_init = init;

        node->nd_foreach_iter = second->nd_lhs;
        node->nd_foreach_content = second->nd_rhs;

        this->expect_semi();

        if (!this->eat_semi())
          node->nd_foreach_cond = this->p_expr();

        if (!this->eat_semi())
          node->nd_foreach_step = this->p_getexpr_rm_block();
      }

      node->nd_for_cond = second;

      this->expect_semi();
    }
  _end_second_parse:;

    if (!this->eat_semi()) {
      try {
        node->nd_for_step = this->p_expr();
      }
      catch (const Error& e) {
        Error(this->cur, "expected expression").crash();
      }
    }

    (node->nd_for_body = this->p_block())->nd_block_parent = node;

    return node;
  }

  //
  // do-while
  //  --> convert to loop-statement
  //
  else if (this->eat(Kwd::Do)) {
    auto dw_body = this->p_block();

    this->expect(Kwd::While);

    auto dw_cond = this->p_expr();

    this->expect_semi();

    // if "not cond" { break; }
    auto ifstmt = Node::new_node(ND_If, this->cur);

    ifstmt->nd_if_cond = Node::new_node(ND_Not, this->cur, dw_cond);

    (ifstmt->nd_if_then = Node::new_node(ND_Block, this->cur))
        ->append(Node::new_node(ND_Break, this->cur));

    auto loop_body = Node::new_node(ND_Block, this->cur);

    for (auto&& dw_elem : dw_body->nd_elements)
      loop_body->append(dw_elem);

    loop_body->append(ifstmt);

    auto loop_node = Node::new_node(ND_Loop, this->cur);
    loop_node->nd_loop_body = loop_body;

    return loop_node;
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
    auto node = Node::new_node(ND_Block, this->cur->prev);

    node->first_tok = this->cur->prev;

    while (!this->match(Punct::BlockBraceClose))
      node->append(this->p_stmt());

    node->last_tok = this->expect(Punct::BlockBraceClose);

    return node;
  }

  return nullptr;
}

} // namespace fire
