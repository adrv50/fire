#pragma once

#include <list>

#include "typedef.h"

namespace fire {

struct Node;

class Evaluator {

  struct CallStack {
    Vec<Obj> objects; // => local variables

    Obj result = nullptr;

    bool pass = false; // pass to end of function body.

    Obj& get(size_t index);

    Obj& append(Obj obj);

    CallStack();
    ~CallStack();
  };

  Vec<Obj> global_variables;

  Vec<CallStack> call_stack;

  Node* program;

  CallStack& get_current_call_stack();

  Obj& push(Obj obj);
  Obj pop();

  CallStack& push_stack();

  void pop_stack();

public:
  Evaluator(Node* program);

  Evaluator(Evaluator&&) = delete;
  Evaluator(Evaluator const&) = delete;

  Obj& append_global_var(Obj obj);

  Obj evaluate();

  Obj eval_expr(Node* node);

  Obj eval_call_func(Node* node, Vec<Obj>& args);

  Obj eval_stmt(Node* node);

  Obj eval_block(Node* node);

  void eval_let(Node* node);
};

} // namespace fire