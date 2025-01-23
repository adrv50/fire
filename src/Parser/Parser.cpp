#include "Debug/Debug.h"
#include "Driver/Error.h"
#include "Token/Token.h"
#include "Node/Node.h"
#include "Parser.h"
#include "Driver/Driver.h"

namespace fire {

Parser::Parser(SourceStorage const& source, Token* tok)
    : cur(tok),
      ate(nullptr),
      source(source) {
}

//
// program ::=
//   root*
//
Node* Parser::parse() {
  auto node = Node::new_node(ND_Program, this->cur);

  node->first_tok = this->cur;

  while (this->check()) {
    if (this->eat(Kwd::Import)) {
      Token* import_tok = this->cur->prev;

      string pathstr;

      if (this->eat(Punct::Dot))
        this->expect(Punct::Slash);

      while (this->check()) {
        Token* tok = this->cur;

        if (this->eat(Punct::PathParentFolder)) {
          pathstr += "../";

          if (!this->eat(Punct::Slash))
            Error(tok, "expected '/' after this token").crash();
        }
        else if (this->eat(TokenKind::Identifier)) {
          if (this->eat(Punct::Slash)) {
            pathstr += tok->str + "/";
            continue;
          }

          pathstr += tok->str + ".fr";

          this->expect_semi();
          break;
        }
        else
          Error(this->cur, "invalid syntax").crash();
      }

      pathstr = std::filesystem::absolute(pathstr).string();

      if (!std::filesystem::exists(pathstr))
        Error(import_tok, "source file '" + pathstr + "' doesn't exists.").crash();

      if (auto imported = this->source.import_source(pathstr)) {
        for (auto&& item : imported->get_parsed()->list) {
          node->append(item);
        }
      }
      else
        Error(import_tok, "'" + pathstr + "' is already opened, ignored this import",
              ErrorType::Warn)
            .emit();
    }
    else
      break;
  }

  while (this->check()) {
    auto nd = node->append(this->p_root());

    if (nd->is(ND_Function) && nd->nd_func_name->str == "main") {
      node->nd_program_main = nd;

      Driver::get_instance()->register_main(nd);
    }
  }

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

  else if ((node = this->p_concept_def()))
    return node;

  else if ((node = this->p_concept_tagged_definition()))
    return node;

  else if ((node = this->p_func()))
    return node;

  else if ((node = this->p_enum()))
    return node;

  else if ((node = this->p_class()))
    return node;

  else if ((node = this->p_struct()))
    return node;

  else if ((node = this->p_namespace()))
    return node;

  else if (this->in_repl)
    return this->p_stmt();

  Error(this->cur,
        "expected definition of variable or function, enum, class, struct, namespace")
      .crash();
}

Node* Parser::p_concept_def() {
  if (!this->eat(Kwd::Concept))
    return nullptr;

  auto node = Node::new_node(ND_Concept, this->cur->prev, nullptr);

  node->first_tok = node->tok;

  node->nd_concept_name = this->expect_ident();

  this->expect(Punct::AngleBraceOpen);

  do {
    node->append(this->p_expect_identifier(false));
  } while (this->eat_comma());

  this->expect(Punct::AngleBraceClose);

  if (this->eat_semi()) {
    node->last_tok = this->cur->prev;
    return node;
  }

  auto ccbody = Node::new_node(ND_ConceptBody, this->expect_block_open(), nullptr);

  ccbody->first_tok = ccbody->tok;

  while (true) {
    auto& ex = ccbody->append(this->p_expr());

    // expr => T
    if (auto _op = this->cur; this->eat(Punct::CaseMatch)) {
      ex = Node::new_node(ND_CCRule_ResultTypeExpection, _op, ex, this->p_expect_type());
    }

    this->expect_semi();

    if (auto b = this->cur; this->eat(Punct::BlockBraceClose)) {
      node->last_tok = ccbody->last_tok = b;
      break;
    }
  }

  node->nd_concept_ccbody = ccbody;

  return node;
}

Node* Parser::p_concept_tagged_definition() {
  Node* cclist = this->eat_concept_tags_list();

  if (!cclist)
    return nullptr;

  Node* node = nullptr;

  if ((node = this->p_concept_def())) {
    node->nd_concept_cclist = cclist;
  }

  else if ((node = this->p_class()))
    node->nd_class_cclist = cclist;

  else if ((node = this->p_func()))
    node->nd_func_cclist = cclist;

  else if ((node = this->p_enum()))
    node->nd_enum_cclist = cclist;

  else
    Error(cclist->last_tok, "expected definition of function or class, struct, enum, "
                            "concept after this token.")
        .crash();

  return node;
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

    if (auto p = this->eat_template_parameter_list()) {
      node->nd_enum_tplist = p;
      node->nd_enum_is_template = true;
    }

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
  if (this->eat_brace_open()) {

    // struct members
    if (this->cur->next && this->cur->next->is_punct(Punct::Colon)) {
      node->nd_enumerator_is_struct = true;
      node->kind = ND_DefEnumeratorWithStructFields;

      do {
        node->append(this->p_struct_member());
      } while (this->eat_comma());
    }
    else {
      // only one type
      node->kind = ND_DefEnumeratorWithValue;
      node->nd_enumerator_is_value = true;
      node->nd_enumerator_val_type = this->p_expect_type();
    }

    this->expect_brace_close();
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

    if (auto p = this->eat_template_parameter_list()) {
      node->nd_struct_tplist = p;
      node->nd_struct_is_template = true;
    }

    this->expect_block_open();

    do {
      node->append(this->p_struct_member());
    } while (this->eat_comma());

    this->expect_block_close();

    return node;
  }

  return nullptr;
}

Node* Parser::p_struct_member() {
  auto member = Node::new_node(ND_StructMember, this->cur);

  member->nd_struct_member_name = this->expect_ident();

  this->expect_colon();

  member->nd_struct_member_type = this->p_expect_type();

  return member;
}

// ---------------------------------
//  p_class:
//    Parse class definition.
// ---------------------------------
Node* Parser::p_class() {
  if (this->eat(Kwd::Class)) {
    auto node = Node::new_node(ND_Class, this->cur->prev);

    node->nd_class_name = this->expect_ident();

    if (auto p = this->eat_template_parameter_list()) {
      node->nd_class_tplist = p;
      node->nd_class_is_template = true;
    }

    this->expect_block_open();

    node->nd_class_fields = Node::new_node(ND_Class_Fields);
    node->nd_class_methods = Node::new_node(ND_Class_Methods);

    while (!this->eat(Punct::BlockBraceClose)) {
      if (auto method = this->p_func()) {
        node->nd_class_methods->append(method);
      }

      else if (auto member = this->p_let()) {
        node->nd_class_fields->append(member);
      }

      else
        Error(this->cur, "expected function or variable declaration").crash();
    }

    if (node->nd_class_fields->list.empty()) {
      Error(node->tok, "no members in class").crash();
    }

    return node;
  }

  return nullptr;
}

Node* Parser::p_namespace() {
  if (this->eat(Kwd::Namespace)) {
    Node* node = Node::new_node(ND_Namespace, this->cur->prev);

    Node* ns = node;

    ns->nd_namespace_name = this->expect_ident();

    while (this->eat(Punct::ScopeResol)) {
      Node* sub = Node::new_node(ND_Namespace, this->cur->prev);
      sub->nd_namespace_name = this->expect_ident();
      ns->append(sub);
      ns = sub;
    }

    this->expect_block_open();

    do {
      ns->append(this->p_root());
    } while (!this->eat(Punct::BlockBraceClose));

    return node;
  }

  return nullptr;
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
    auto tok = this->cur;

    auto node = Node::new_node(ND_Function, this->cur);

    node->first_tok = tok;

    node->nd_func_name = this->expect_ident();

    if (auto tplist = this->eat_template_parameter_list()) {
      node->nd_func_tplist = tplist;
      node->nd_func_is_template = true;
    }

    this->expect_brace_open();

    if (!this->eat_brace_close()) {
      do {
        node->append(this->p_func_arg());
      } while (this->eat_comma());

      this->expect_brace_close();
    }

    if (this->eat(Punct::ResultTypeSpecifier))
      node->nd_func_result_type = this->p_expect_type();

    if (this->eat(Punct::CaseMatch)) {
      tok = this->cur->prev;

      auto expr = this->p_expr();
      this->expect_semi();

      node->nd_func_body = Node::new_node(ND_Block, tok);
      node->nd_func_body->first_tok = tok;
      node->nd_func_body->append(Node::new_node(ND_Return, tok->next))->nd_return_expr =
          expr;

      node->nd_func_is_one_line = true;
    }
    else {
      (node->nd_func_body = this->p_block(true))->nd_block_parent = node;
    }

    node->last_tok = this->cur->prev;

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

  this->expect_colon();

  node->nd_func_arg_type = this->p_expect_type();

  return node;
}

} // namespace fire