#pragma once

#include <optional>

#include "AST.h"

namespace fire::sema {

template <class T>
using vec = std::vector<T>;

template <class T>
using Node = ASTPtr<T>;

template <class T>
using WeakVec = vec<std::weak_ptr<T>>;

using TypeVec = vec<TypeInfo>;

using namespace AST;

class Sema {

  enum class NameType {
    Unknown,
    Var,
    Func,
    Enum,
    Class,
  };

  struct NameFindResult {
    NameType type = NameType::Unknown;

    std::string name;

    Node<VarDef> ast_var_decl; // if variable
    WeakVec<Function> ast_function;
    Node<Enum> ast_enum;
    Node<Class> ast_class;
  };

  struct ArgumentCheckResult {
    enum Result {
      Ok,
      TooFewArguments,
      TooManyArguments,
      TypeMismatch,
    };

    Result result;
    int index;

    ArgumentCheckResult(Result r, int i = 0)
        : result(r),
          index(i) {
    }
  };

  ArgumentCheckResult
  check_function_call_parameters(Node<CallFunc> call, bool isVariableArg,
                                 TypeVec const& formal,
                                 TypeVec const& actual);

public:
  Sema(Node<AST::Program> prg);

  void check();

  TypeInfo evaltype(ASTPointer ast);

  NameFindResult find_name(std::string const& name);

private:
  auto& add_func(Node<Function> f) {
    return this->functions.emplace_back(f);
  }

  auto& add_enum(Node<Enum> e) {
    return this->enums.emplace_back(e);
  }

  auto& add_class(Node<Class> c) {
    return this->classes.emplace_back(c);
  }

  Node<Program> root;

  WeakVec<Function> functions;
  WeakVec<Enum> enums;
  WeakVec<Class> classes;
};

} // namespace fire::sema