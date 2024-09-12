#pragma once

#include "Object.h"

namespace metro::eval {

template <class T>
using ObjPair = std::pair<ObjPtr<T>, ObjPtr<T>>;

using PrimitiveObjPair = ObjPair<ObjPrimitive>;

template <class T, class U>
static inline ObjPair<T> pair_ptr_cast(std::shared_ptr<U> a,
                                       std::shared_ptr<U> b) {
  static_assert(!std::is_same_v<T, U>);
  return std::make_pair(PtrCast<T>(a), PtrCast<T>(b));
}

template <class T>
static inline PrimitiveObjPair to_primitive_pair(ObjPtr<T> a,
                                                 ObjPtr<T> b) {
  return pair_ptr_cast<ObjPrimitive>(a, b);
}

class Evaluator {

  struct VarTable {
    std::map<std::string, ObjPointer> map;

    // for return-statement
    ObjPointer result = nullptr;

    VarTable() {
    }
  };

public:
  Evaluator(AST::Program ast);

  void do_eval();

  ObjPointer eval_expr(ASTPtr<AST::Expr> ast);
  ObjPointer eval_member_access(ASTPtr<AST::Expr> ast);

  ObjPointer evaluate(ASTPointer ast);

private:
  ObjPointer* find_var(std::string const& name);

  std::pair<ASTPtr<AST::Function>, builtins::Function const*>
  find_func(std::string const& name);

  VarTable& get_cur_vartable() {
    return *this->stack.rbegin();
  }

  VarTable& push() {
    return this->stack.emplace_back();
  }

  void pop() {
    this->stack.pop_back();
  }

  static void adjust_numeric_type_object(ObjPtr<ObjPrimitive> left,
                                         ObjPtr<ObjPrimitive> right);

  AST::Program root;
  std::vector<ASTPtr<AST::Function>> functions;

  std::vector<VarTable> stack;
};

} // namespace metro::eval