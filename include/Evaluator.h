#pragma once

#include <list>

#include "AST.h"
#include "Object.h"

namespace fire::eval {

class Evaluator {

public:
  Evaluator();
  ~Evaluator();

  ObjPointer evaluate(ASTPointer ast);
  ObjPointer eval_expr(ASTPtr<AST::Expr> ast);

  ObjPointer& eval_as_left(ASTPointer ast);

  ObjPointer& eval_index_ref(ObjPointer array, ObjPointer index);

private:
  struct VarStack {
    vector<ObjPointer> var_list;

    bool returned = false;
    ObjPointer func_result = nullptr;
  };

  VarStack& push_stack(int var_size);
  void pop_stack();

  VarStack& get_cur_stack();
  VarStack& get_stack(int distance);

  std::list<std::shared_ptr<VarStack>> var_stack;
};

} // namespace fire::eval