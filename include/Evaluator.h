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

    bool breaked = false;
    bool continued = false;

    VarStack(size_t vcount) {
      this->var_list.reserve(vcount);
    }
  };

  using VarStackPtr = std::shared_ptr<VarStack>;

  VarStackPtr push_stack(size_t var_count);
  void pop_stack();

  VarStack& get_cur_stack();
  VarStack& get_stack(int distance);

  std::list<VarStackPtr> var_stack;

  std::list<VarStackPtr> call_stack;
  std::list<VarStackPtr> loops;
};

} // namespace fire::eval