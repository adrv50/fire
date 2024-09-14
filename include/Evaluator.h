#pragma once

#include <list>
#include "Object.h"

#define EVALUATOR_STACK_MAX_SIZE 416

namespace metro::eval {

template <class T>
using ObjPair = std::pair<ObjPtr<T>, ObjPtr<T>>;

using PrimitiveObjPair = ObjPair<ObjPrimitive>;

template <class T, class U>
static inline ObjPair<T> pair_ptr_cast(ObjPtr<U> a, ObjPtr<U> b) {
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

      LocalVar(std::string const& name, ObjPointer val)
          : name(name),
            value(val) {
      }
    };

    std::vector<LocalVar> localvar;

    // for return-statement
    ObjPointer result = nullptr;

    bool is_returned = false;

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

  struct LoopContext {
    ASTPointer ast;

    bool is_breaked = false;
    bool is_continued = false;

    LoopContext(ASTPointer ast)
        : ast(ast) {
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

  ObjPointer call_function_ast(bool have_self,
                               ASTPtr<AST::Function> ast,
                               ASTPtr<AST::CallFunc> call,
                               ObjVector& args);

  EvalStack& GetCurrentStack() {
    return *this->stack.rbegin();
  }

  EvalStack& PushStack() {
    return this->stack.emplace_back();
  }

  void PopStack() {
    this->stack.pop_back();
  }

  LoopContext& EnterLoopStatement(ASTPtr<AST::Statement> ast);
  void LeaveLoop();

  LoopContext& GetCurrentLoop();

  static void adjust_numeric_type_object(ObjPtr<ObjPrimitive> left,
                                         ObjPtr<ObjPrimitive> right);

  static auto adjust_numeric_type_object(ObjPair<ObjPrimitive> pair) {
    adjust_numeric_type_object(pair.first, pair.second);
    return pair;
  }

  ASTPtr<AST::Program> root;

  ASTVec<AST::Function> functions;
  ASTVec<AST::Class> classes;

  std::list<EvalStack> stack;

  std::vector<LoopContext> loop_stack;
};

} // namespace metro::eval