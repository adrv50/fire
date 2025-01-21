#pragma once

#include "alert.h"
#include "node2s.h"

namespace fire::Debug {

Node* make_nd_root(Vec<Node*>&& nodes);

Node* make_nd_func(char const* name, Vec<pair<char const*, Node*>>&& args, Node* body);

Node* make_nd_func(char const* name, Vec<pair<char const*, Node*>>&& args,
                   Node* return_type, Node* body);

Node* make_nd_if(Node* cond, Node* then, Node* Else = nullptr);
Node* make_nd_let(char const* name, Node* type, Node* init);
Node* make_nd_block(Vec<Node*>&& nodes);

Node* make_nd_type(char const* name);
Node* make_nd_type(char const* name, Vec<Node*>&& tp_args, bool Mut = false,
                   bool Ref = false);
Node* make_nd_type(Vec<char const*>&& scope_resol, Vec<Node*>&& tp_args, bool Mut = false,
                   bool Ref = false);

Node* make_nd_expr(NodeKind kind, Node* lhs, Node* rhs);
Node* make_nd_scope_resol(Vec<char const*>&& scope_resol);
Node* make_nd_id(char const* name);
Node* make_nd_val(Object* obj);

} // namespace fire::Debug