#include "alert.h"
#include "Error.h"
#include "Token.h"
#include "Node.h"
#include "Parser.h"

//
// program ::=
//   root*
//
Node* Parser::parse() {
  auto node = Node::new_node(ND_Program, this->cur);

  node->first_tok = this->cur;

  while (this->check())
    node->append(this->p_root());

  node->last_tok = this->cur->prev;

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

  else if ((node = this->p_enum()))
    return node;

  else if ((node = this->p_class()))
    return node;

  else if ((node = this->p_struct()))
    return node;

  else if (this->in_repl)
    return this->p_stmt();

  Error(this->cur,
        "expected definition of variable or function, enum, class, struct, namespace")
      .crash();
}

// -----------------------------------------------
// p_enum:
//  Parse enum definition.
//
// *---
// BNF
//   enum ::=
//     "enum" ident "{" def_enumerator ("," def_enumerator)* "}"
//
// *---
// structure map
//   def_enumerator   --> nd_enum_enumerators[N]
//   en_val           --> nd_enum_enumerators[N].nd_enumerator_is_value
//   en_struct        --> nd_enum_enumerators[N].nd_enumerator_struct_members[N]
// -----------------------------------------------
Node* Parser::p_enum() {
  if (this->eat(Kwd::Enum)) {
    auto node = Node::new_node(ND_Enum, this->cur);

    node->first_tok = this->cur;

    node->nd_enum_name = this->expect_ident();

    this->expect(Punct::BlockBraceOpen);

    do {
      node->append(this->p_def_enumerator());
    } while (this->eat(Punct::Comma));

    node->last_tok = this->cur;

    this->expect(Punct::BlockBraceClose);

    return node;
  }

  return nullptr;
}

// -----------------------------------------------
// p_def_enumerator:
//   Parse enumerator definition.
//
// *---
// BNF
//   def_enumerator ::=
//     ident ("(" en_val | en_struct ")")?
//
//   en_val ::=
//     type
//
//   en_struct ::=
//     "{" struct_member ("," struct_member)* "}"
//
//   struct_member ::=
//     ident ":" type
// -----------------------------------------------
Node* Parser::p_def_enumerator() {
  auto tok = this->cur;

  auto node = Node::new_node(ND_DefEnumerator, this->cur);

  node->first_tok = tok;

  node->nd_enumerator_name = this->expect_ident();

  // have a data
  if (this->eat(Punct::BraceOpen)) {

    // struct members
    if (this->cur->next->is_punct(Punct::Colon)) {
      node->nd_enumerator_is_struct = true;

      do {
        node->append(this->p_struct_member());
      } while (this->eat(Punct::Comma));
    }
    else {
      // only type
      node->nd_enumerator_is_value = true;
      node->nd_enumerator_val_type = this->p_expect_type();
    }

    this->expect(Punct::BraceClose);
  }

  node->last_tok = this->cur->prev;

  return node;
}

// ---------------------------------
// p_struct:
//   Parse struct definition.
// ---------------------------------
Node* Parser::p_struct() {
  if (this->eat(Kwd::Struct)) {
    auto node = Node::new_node(ND_Struct, this->cur);

    node->nd_struct_name = this->expect_ident();

    this->expect(Punct::BlockBraceOpen);

    do {
      node->append(this->p_struct_member());
    } while (this->eat(Punct::Comma));

    this->expect(Punct::BlockBraceClose);
  }

  return nullptr;
}

Node* Parser::p_struct_member() {
  auto member = Node::new_node(ND_StructMember, this->cur);

  member->nd_struct_member_name = this->expect_ident();

  this->expect(Punct::Colon);

  member->nd_struct_member_type = this->p_expect_type();

  return member;
}

// ---------------------------------
//  p_class:
//    Parse class definition.
// ---------------------------------
Node* Parser::p_class() {
  if (this->eat(Kwd::Class)) {
    auto node = Node::new_node(ND_Class, this->cur);

    node->nd_class_name = this->expect_ident();

    this->expect(Punct::BlockBraceOpen);

    while (!this->eat(Punct::BlockBraceClose)) {
      if (auto fn = this->p_func()) {
        node->append(fn);
        continue;
      }

      if (auto let = this->p_let()) {
        node->append(let);
        continue;
      }

      Error(this->cur, "expected function or variable declaration").crash();
    }

    return node;
  }

  return nullptr;
}

Node* Parser::p_namespace() {
  todo_impl;
}

// ---------------------------------
// p_func:
//   Parse function definition.
//
// *---
// BNF
//   func ::=
//     func ident "(" func_args ("," func_args)* ")" ("->" type)? block
// ---------------------------------
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
