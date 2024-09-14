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

  struct EvalStack {
    struct LocalVar {
      std::string name;
      ObjPointer value;
    };

    std::vector<LocalVar> localvar;

    // for return-statement
    ObjPointer result = nullptr;

    LocalVar* find_variable(std::string const& name) {
      for (auto&& lvar : this->localvar) {
        if (lvar.name == name)
          return &lvar;
      }

      return nullptr;
    }

    LocalVar& append(std::string const& name,
                     ObjPointer val = nullptr) {
      return this->localvar.emplace_back(name, val);
    }

    EvalStack() {
    }
  };

public:
  Evaluator(ASTPtr<AST::Program> ast);

  void do_eval();

  ObjPointer eval_expr(ASTPtr<AST::Expr> ast);
  ObjPointer eval_member_access(ASTPtr<AST::Expr> ast);

  ObjPointer& eval_as_writable(ASTPointer ast);

  ObjPointer evaluate(ASTPointer ast);

private:
  ObjPointer* find_var(std::string const& name);

  std::pair<ASTPtr<AST::Function>, builtins::Function const*>
  find_func(std::string const& name);

  ASTPtr<AST::Class> find_class(std::string const& name);

  ObjPtr<ObjInstance> new_class_instance(ASTPtr<AST::Class> ast);

  ObjPointer call_function_ast(ASTPtr<AST::Function> ast,
                               ASTPtr<AST::CallFunc> call,
                               ObjVector& args);

  EvalStack& get_cur_stack() {
    return *this->stack.rbegin();
  }

  EvalStack& push() {
    return this->stack.emplace_back();
  }

  void pop() {
    this->stack.pop_back();
  }

  static void adjust_numeric_type_object(ObjPtr<ObjPrimitive> left,
                                         ObjPtr<ObjPrimitive> right);

  ASTPtr<AST::Program> root;

  ASTVec<AST::Function> functions;
  ASTVec<AST::Class> classes;

  std::vector<EvalStack> stack;
};

} // namespace metro::eval