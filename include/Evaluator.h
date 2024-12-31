#pragma once

#include <list>

#include "Node.h"

class Evaluator {

  struct CallStack {
    Vec<Obj> objects;

    Obj result;

    bool is_returned;

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

  Obj evaluate();

  Obj eval_expr(Node* node);

  Obj eval_call_func(Node* node, Vec<Obj>& args);

  void eval_stmt(Node* node);

  void eval_block(Node* node);

  void eval_let(Node* node);
};
