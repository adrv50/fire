#include "alert.h"
#include "Node.h"

bool Node::is(NodeKind kind) const {
  return this->kind == kind;
}

string Node::get_name() const {
  switch (this->kind) {
  case ND_Identifier:
    return this->tok->str;

  case ND_Function:
    return this->nd_func_name->str;

  case ND_Enum:
    return this->nd_enum_name->str;

  case ND_Class:
    return this->nd_class_name->str;

  case ND_Struct:
    return this->nd_struct_name->str;

  default:
    todo_impl;
  }

  return "";
}

Node*& Node::append(Node* node) {
  return this->list.emplace_back(node);
}

Node* Node::new_node(NodeKind kind, Token* tok) {
  return new Node(kind, tok);
}

Node* Node::new_node(NodeKind kind, Token* tok, Node* lhs, Node* rhs) {
  return new Node(kind, tok, lhs, rhs);
}

Node* Node::new_value(Token* tok, Object* obj) {
  return new Node(ND_Value, tok, obj);
}

Node::Node(NodeKind kind, Token* tok, Object* obj)
    : kind(kind),
      tok(tok) {
  this->nd.obj = obj;
}

Node::Node(NodeKind kind, Token* tok, Node* lhs, Node* rhs)
    : Node(kind, tok) {
  this->nd_lhs = lhs;
  this->nd_rhs = rhs;
}

Node::~Node() {
  switch (this->kind) {
  case ND_Value:
    delete this->nd.obj;
    break;
  }
}
