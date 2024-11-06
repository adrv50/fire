#pragma once

#include <concepts>

namespace fire::AST {

struct Enum : Templatable {
  struct Enumerator {
    enum class DataType {
      NoData,
      Value,     // Kind(T)
      Structure, // Kind(a: T, b: U)
    };

    Token name;

    DataType data_type = DataType::NoData;

    //
    // usage of types:
    //   NoData     => (empty)
    //   Value      => types[0] is ASTPtr<TypeName>
    //   Structure  => types is ASTVec<Argument>
    ASTVector types;
  };

  vector<Enumerator> enumerators;

  template <typename... Args>
  requires std::constructible_from<Enumerator, Args...>
  Enumerator& append(Args&&... args) {
    return this->enumerators.emplace_back(std::forward<Args>(args)...);
  }

  static ASTPtr<Enum> New(Token tok, Token name);

  ASTPointer Clone() const override;

  Enum(Token tok, Token name);
};

} // namespace fire::AST