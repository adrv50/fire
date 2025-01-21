#include "Debug/Debug.h"
#include "Token/Token.h"

namespace fire {

extern size_t tok_operators_size;
extern size_t tok_punctuators_size;
extern size_t tok_keywords_size;

extern pair<TokenOperatorKind, char const*> tok_operators[];
extern pair<TokenPunctKind, char const*> tok_punctuators[];
extern pair<TokenKwdKind, char const*> tok_keywords[];

} // namespace fire

namespace fire::Debug {

static Token* make_tok(string const& str, TokenKind kind = TokenKind::Identifier) {
  Token* tok = Token::make(kind, nullptr, nullptr, str, 0);

  for (size_t i = 0; i < tok_operators_size; i++)
    if (auto const& [k, s] = tok_operators[i]; str == s) {
      tok->op = k;
      break;
    }

  for (size_t i = 0; i < tok_punctuators_size; i++)
    if (auto const& [k, s] = tok_punctuators[i]; str == s) {
      tok->punct = k;
      break;
    }

  for (size_t i = 0; i < tok_keywords_size; i++)
    if (auto const& [k, s] = tok_keywords[i]; str == s) {
      tok->kwd = k;
      break;
    }

  return tok;
}

Node* make_nd_root(Vec<Node*>&& nodes) {
  Node* root = Node::new_node(ND_Program);

  root->list = nodes;

  return root;
}

Node* make_nd_func(char const* name, Vec<pair<char const*, Node*>>&& args, Node* body) {
  return make_nd_func(name, std::move(args), nullptr, body);
}

Node* make_nd_func(char const* name, Vec<pair<char const*, Node*>>&& args,
                   Node* return_type, Node* body) {
  Node* node = Node::new_node(ND_Function);

  node->nd_func_name = make_tok(name);

  for (auto&& [name, type] : args) {
    auto arg = node->append(Node::new_node(ND_FunctionArg));

    arg->nd_func_arg_name = make_tok(name);
    arg->nd_func_arg_type = type;
  }

  node->nd_func_result_type = return_type;
  node->nd_func_body = body;

  return node;
}

Node* make_nd_if(Node* cond, Node* then, Node* Else) {
  Node* if_ = Node::new_node(ND_If);

  if_->nd_if_cond = cond;
  if_->nd_if_then = then;
  if_->nd_if_else = Else;

  return if_;
}

Node* make_nd_let(char const* name, Node* type, Node* init) {
  Node* let = Node::new_node(ND_Let);

  let->nd_let_name = make_tok(name);
  let->nd_let_type = type;
  let->nd_let_init = init;

  return let;
}

Node* make_nd_block(Vec<Node*>&& nodes) {
  Node* block = Node::new_node(ND_Block);

  block->list = nodes;

  return block;
}

Node* make_nd_type(char const* name) {
  Node* type = Node::new_node(ND_TypeName);

  type->nd_type_id = make_nd_id(name);

  return type;
}

Node* make_nd_type(char const* name, Vec<Node*>&& tp_args, bool Mut, bool Ref) {
  auto nd = make_nd_type(name);

  nd->nd_type_tp_args_ptr = Node::new_node(ND_TemplateArguments);
  nd->nd_type_tp_args = std::move(tp_args);

  nd->nd_type_is_mut = Mut;
  nd->nd_type_is_ref = Ref;

  return nd;
}

Node* make_nd_type(Vec<char const*>&& scope_resol, Vec<Node*>&& tp_args, bool Mut,
                   bool Ref) {
  auto nd = make_nd_type(scope_resol[0]);

  for (size_t i = 1; i < scope_resol.size(); i++)
    nd->append(make_nd_id(scope_resol[i]));

  nd->nd_type_tp_args_ptr = Node::new_node(ND_TemplateArguments);
  nd->nd_type_tp_args = std::move(tp_args);

  nd->nd_type_is_mut = Mut;
  nd->nd_type_is_ref = Ref;

  return nd;
}

Node* make_nd_expr(NodeKind kind, Node* lhs, Node* rhs) {
  return Node::new_node(kind, nullptr, lhs, rhs);
}

Node* make_nd_scope_resol(Vec<char const*>&& scope_resol) {
  Node* sr = Node::new_node(ND_ScopeResol);

  sr->nd_scope_resol_first = make_nd_id(scope_resol[0]);

  for (size_t i = 1; i < scope_resol.size(); i++)
    sr->append(make_nd_id(scope_resol[i]));

  return sr;
}

Node* make_nd_id(char const* name) {
  Node* id = Node::new_node(ND_Identifier);

  id->nd_id_name = make_tok(name, TokenKind::Identifier);

  return id;
}

Node* make_nd_val(Object* obj) {
  Node* val = Node::new_node(ND_Value);

  val->nd_value = obj;

  return val;
}

} // namespace fire::Debug