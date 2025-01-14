#include "alert.h"
#include "Token.h"
#include "Node.h"
#include "Object.h"

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

Node* Node::clone() {

  auto cloned = new Node(this->kind, this->tok->clone(), this->obj);

  cloned->tok2 = this->tok2;
  cloned->tok3 = this->tok3;

  cloned->b1 = this->b1;
  cloned->b2 = this->b2;
  cloned->b3 = this->b3;
  cloned->b4 = this->b4;

  Node* bases[] = {this->na, this->nb, this->nc, this->nd, this->ne, this->nf};

  if (this->kind == ND_Block)
    bases[0] = nullptr;

  for (size_t i = 0; auto& c : {&cloned->na, &cloned->nb, &cloned->nc, &cloned->nd,
                                &cloned->ne, &cloned->nf}) {
    if (bases[i])
      *c = (bases[i])->clone();

    i++;
  }

  for (auto& x : this->list)
    cloned->append(x->clone());

  return cloned;
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

bool Node::walk_node(Node* nd, std::function<bool(Node*&)> const& func) {
  if (!nd)
    return false;

  if (func(nd))
    return true;

  auto tree = nd->list;

  for (auto&& xx : {nd->na, nd->nb, nd->nc, nd->nd, nd->ne, nd->nf})
    tree.push_back(xx);

  if (nd->is(ND_Block)) {
    tree.erase(std::find(tree.begin(), tree.end(), nd->nd_block_parent));
  }

  for (auto& node : tree)
    if (walk_node(node, func))
      return true;

  return false;
}

Node::Node(NodeKind kind, Token* tok, Object* obj)
    : kind(kind),
      tok(tok) {
  this->obj = obj;
}

Node::Node(NodeKind kind, Token* tok, Node* lhs, Node* rhs)
    : Node(kind, tok) {
  this->nd_lhs = lhs;
  this->nd_rhs = rhs;
}

Node::~Node() {
}
