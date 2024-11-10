#pragma once

#include <list>

#include "AST.h"
#include "Object.h"

namespace fire::eval {

struct VarStack {
  vector<ObjPointer> var_list;

  bool returned = false;
  ObjPointer func_result = nullptr;

  bool breaked = false;
  bool continued = false;

  VarStack(size_t vcount) {
    this->var_list.resize(vcount);
  }
};

class Evaluator {

  semantics_checker::Sema& S;

public:
  Evaluator(semantics_checker::Sema& S);
  ~Evaluator();

  ObjPointer evaluate(ASTPointer ast);

  ObjPointer eval_expr(ASTPtr<AST::Expr> ast);
  void eval_stmt(ASTPointer ast);

  ObjPointer& eval_as_left(ASTPointer ast);

  ObjPointer& eval_index_ref(ASTPtr<AST::Expr> ast, ObjPointer array, ObjPointer index);

  //
  ObjPointer& eval_member_ref(ObjPtr<ObjInstance> inst, ASTPtr<AST::Class> expected_class,
                              int index);

private:
  using VarStackPtr = std::shared_ptr<VarStack>;

  ObjPtr<ObjInstance> CreateClassInstance(ASTPtr<AST::Class> ast);

  ObjPointer MakeDefaultValueOfType(TypeInfo const& type);

  VarStackPtr push_stack(size_t var_count);
  void pop_stack();

  VarStack& get_cur_stack();
  VarStack& get_stack(int distance);

  std::list<VarStackPtr> var_stack;

  std::list<VarStackPtr> call_stack;
  std::list<VarStackPtr> loops;

  static ObjPtr<ObjNone> _None;
};

} // namespace fire::eval