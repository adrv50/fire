#pragma once

#include "AST.h"

namespace metro {

class Lexer {

public:
  Lexer(SourceStorage const& source);

  TokenVector Lex();

private:
  bool check() const;
  char peek();
  void pass_space();
  bool match(std::string_view);

  bool eat(char c) {
    if (this->peek() == c) {
      this->position++;
      return true;
    }

    return false;
  }

  bool eat(std::string_view s) {
    if (this->match(s)) {
      this->position += s.length();
      return true;
    }

    return false;
  }

  std::string_view trim(i64 len) {
    return std::string_view(this->source.data.data() + this->position,
                            len);
  }

  SourceStorage const& source;
  i64 position;
  i64 length;
};

} // namespace metro

namespace metro::parser {

template <std::derived_from<AST::Base> T, class... Args>
requires std::constructible_from<T, Args...>
std::shared_ptr<T> ASTNew(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

class Parser {

  struct IdentifierTag {
    enum Type {
      Unknown = -1,
      Variable = 0,
      Namespace,
      Enum,
      Class,
      Function,
    };

    Type type;
    TokenIterator tok;
    int scope_depth;

    IdentifierTag(Type type, TokenIterator tok, int scope_depth)
        : type(type), tok(tok), scope_depth(scope_depth) {
    }

    bool can_use_scope_resol_op() const {
      switch (this->type) {
      case Type::Namespace:
      case Type::Enum:
      case Type::Class:
        return true;
      }

      return false;
    }

    bool is_type() {
      switch (this->type) {
      case Enum:
      case Class:
      case Function:
        return true;
      }

      return false;
    }
  };

public:
  Parser(TokenVector tokens);

  // ASTPointer Ident();

  ASTPointer Factor();

  ASTPointer IndexRef();
  ASTPointer Unary();

  ASTPointer Mul();
  ASTPointer Add();
  ASTPointer Shift();
  ASTPointer Compare();
  ASTPointer BitCalc();
  ASTPointer LogAndOr();
  ASTPointer Assign();

  ASTPointer Expr();
  ASTPointer Stmt();

  ASTPointer Top();

  AST::Program Parse();

private:
  bool check() const;

  bool eat(std::string_view str);
  void expect(std::string_view str, bool no_next = false);

  bool match(std::string_view s) {
    return this->cur->str == s;
  }

  bool match(TokenKind kind) {
    return this->cur->kind == kind;
  }

  bool match(std::pair<TokenKind, std::string_view> pair) {
    return this->match(pair.first) && this->match(pair.second);
  }

  template <class T, class U, class... Args>
  bool match(T&& t, U&& u, Args&&... args) {
    if (!this->check() || !this->match(std::forward<T>(t))) {
      return false;
    }

    auto save = this->cur;
    this->cur++;

    auto ret =
        this->match(std::forward<U>(u), std::forward<Args>(args)...);

    this->cur = save;
    return ret;
  }

  static ASTPtr<AST::Expr> new_expr(ASTKind k, Token& op,
                                    ASTPointer lhs, ASTPointer rhs) {
    return ASTNew<AST::Expr>(k, op, lhs, rhs);
  }

  static ASTPtr<AST::Expr> new_assign(ASTKind kind, Token& op,
                                      ASTPointer lhs,
                                      ASTPointer rhs) {
    return new_expr(ASTKind::Assign, op, lhs,
                    new_expr(kind, op, lhs, rhs));
  }

  TokenIterator insert_token(Token tok) {
    this->cur = this->tokens.insert(this->cur, tok);
    this->end = this->tokens.end();

    return this->cur;
  }

  TokenIterator expectIdentifier();

  ASTPtr<AST::TypeName> expectTypeName();

  void MakeIdentifierTags();

  int find_name(std::vector<IdentifierTag>& out,
                std::string_view name) {
    for (auto&& [n, t] : this->id_tag_map) {
      if (n == name && t.scope_depth <= this->cur_scope_depth)
        out.emplace_back(t);
    }

    return (int)out.size();
  }

  bool is_type_name(std::string_view name) {
    for (auto&& [n, t] : this->id_tag_map) {
      if (n == name && t.scope_depth <= this->cur_scope_depth) {
        if (t.is_type())
          return true;
      }
    }

    return TypeInfo::is_primitive_name(name);
  }

  IdentifierTag& add_id_tag(std::string_view name,
                            IdentifierTag&& tag) {
    return this->id_tag_map.emplace_back(name, std::move(tag)).second;
  }

  TokenVector tokens;
  TokenIterator cur, end, ate;

  int cur_scope_depth = 0;

  std::vector<std::pair<std::string_view, IdentifierTag>> id_tag_map;
};

} // namespace metro::parser