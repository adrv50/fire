#pragma once

#include "alert.h"
#include "node2s.h"

namespace fire::Debug {

Node* make_node_root();
Node* make_node_if(Node* cond, Node* then, Node* Else = nullptr);

} // namespace fire::Debug