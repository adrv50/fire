#include "Debug/alert.h"
#include "Token/Token.h"
#include "Node/Node.h"
#include "Object.h"

namespace fire {

bool Node::is(NodeKind kind) const {
  return this->kind == kind;
}

bool Node::is_id_or_sr() const {
  return this->is(ND_Identifier) || this->is(ND_ScopeResol);
}

bool Node::is_loop_stmt() const {
  switch (this->kind) {
    case ND_Loop:
    case ND_While:
    case ND_For:
    case ND_ForEach:
    case ND_ForRange:
      return true;
  }

  return false;
}

bool Node::is_named_node() const {
  switch (this->kind) {
    case ND_Identifier:
    case ND_Function:
    case ND_Enum:
    case ND_Class:
    case ND_Struct:
    case ND_Let:
      return true;
  }

  return false;
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

    case ND_Let:
      return this->nd_let_name->str;

    default:
      todo_impl;
  }

  throw std::logic_error("get_name() called but node is not named. (or not implemented)");
}

Node*& Node::append(Node* node) {
  return this->list.emplace_back(node);
}

Node* Node::get_enumerator(size_t index) const {
  return this->nd_enum_enumerators[index];
}

Node* Node::clone() {

  auto cloned = new Node(this->kind, this->tok->clone(), this->obj);

  cloned->tok2 = this->tok2;
  cloned->tok3 = this->tok3;

  cloned->b1 = this->b1;
  cloned->b2 = this->b2;
  cloned->b3 = this->b3;
  cloned->b4 = this->b4;

  cloned->v1 = this->v1;
  cloned->v2 = this->v2;
  cloned->v3 = this->v3;
  cloned->v4 = this->v4;

  cloned->size = this->size;
  cloned->size2 = this->size2;

  cloned->bfun = this->bfun;
  cloned->sema_ctx = this->sema_ctx;
  cloned->sym = this->sym;

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

Node* Node::new_compare(CompareExprKind ck, Token* op, Node* lhs, Node* rhs) {
  auto nd = Node::new_node(ND_Compare, op, lhs, rhs);

  nd->cmp_kind = ck;

  return nd;
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

Node* Node::get_last_id() {
  if (this->is(ND_Identifier))
    return this;

  return this->nd_scope_resol_idlist.back();
}

} // namespace fire