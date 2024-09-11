#pragma once

#include <any>
#include <algorithm>
#include <concepts>
#include <string>
#include "Token.h"
#include "Evaluator/Object.h"

namespace metro {

enum class ASTKind {
  Value,

  Identifier,

  ScopeResolution,

  IndexRef,
  MemberAccess,
  CallFunc,

  Mul,
  Div,
  Add,
  Sub,

  Block,
  Vardef,

  If,
  Switch,
  For,

  FuncArg,
  Function,

  Enumerator,
  Enum,

  Class,

  Module,

  Namespace,

  TypeName,
};

namespace AST {

struct Base {
  ASTKind kind;
  Token token;
  Token endtok;

  bool is_named = false;

  template <class T>
  T* As() {
    return static_cast<T*>(this);
  }

  template <class T>
  T const* As() const {
    return static_cast<T*>(this);
  }

  virtual std::string_view GetSourceView() const {
    return token.sourceloc.GetLine();
  }

  virtual ~Base() = default;

protected:
  Base(ASTKind kind, Token const& token, Token endtok = {})
      : kind(kind), token(token), endtok(endtok) {
  }
};

struct Value : Base {
  ObjPointer value;

  Value(Token tok, ObjPointer value)
      : Base(ASTKind::Value, tok, tok), value(value) {
  }
};

struct Identifier : Base {
  Token& name;
  ASTVector id_params;

  Identifier(Token name)
      : Base(ASTKind::Identifier, name, name), name(this->token) {
  }
};

struct CallFunc : Base {
  ASTPointer expr; // left side
  ASTVector args;

  CallFunc(ASTPointer expr)
      : Base(ASTKind::CallFunc, expr->token, expr->token),
        expr(expr) {
  }
};

struct Expr : Base {
  Token& op;
  ASTPointer lhs;
  ASTPointer rhs;

  Expr(ASTKind kind, Token op, ASTPointer lhs, ASTPointer rhs)
      : Base(kind, op), op(this->token), lhs(lhs), rhs(rhs) {
  }
};

struct Block : Base {
  ASTVector list;

  Block(ASTVector list = {}) : Base(ASTKind::Block, "{", "}") {
  }
};

struct Named : Base {
  Token name;

  std::string_view GetName() const {
    return this->name.str;
  }

protected:
  Named(ASTKind kind, Token tok, Token name)
      : Base(kind, tok), name(name) {
    this->is_named = true;
  }
};

struct Namespace : Named {
  ASTVector list;

  Namespace(Token tok, Token name)
      : Named(ASTKind::Namespace, tok, tok) {
  }
};

struct VarDef : Named {
  ASTPtr<TypeName> type;
  ASTPointer init;

  VarDef(Token tok, Token name) : Named(ASTKind::Vardef, tok, name) {
  }
};

struct Statement : Base {
  // ASTKind::If
  struct If {
    ASTPointer cond, if_true, if_false;
  };

  struct Switch {
    struct Case {
      ASTPointer expr, block;
    };

    ASTPointer cond;
    std::vector<Case> cases;
  };

  struct For {
    ASTPointer init, cond, count, block;
  };

  std::any astdata;

  Statement(ASTKind kind, Token tok, std::any data)
      : Base(kind, tok), astdata(data) {
  }
};

struct TypeName : Base {
  Token& name;
  ASTVector type_params;
  ASTVec<TypeName> scope_resol;
  bool is_const;

  TypeInfo type;
  std::weak_ptr<Class> ast_class;
  builtin::Class const* builtin_class;

  TypeName(Token name)
      : Base(ASTKind::TypeName, name), name(this->token),
        is_const(false), builtin_class(nullptr) {
  }
};

struct Templatable : Named {
  ASTVec<Identifier> template_params;

  bool IsTemplated() const {
    return !this->template_params.empty();
  }

protected:
  Templatable(ASTKind kind, Token tok, Token name)
      : Named(kind, tok, name) {
  }
};

struct FuncArgument : Named {
  ASTPtr<TypeName> type;

  FuncArgument(Token tok, ASTPtr<TypeName> type)
      : Named(ASTKind::FuncArg, tok, tok), type(type) {
  }
};

struct Function : Templatable {
  ASTVec<FuncArgument> args;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  Function(Token tok, Token name)
      : Templatable(ASTKind::Function, tok, name) {
  }
};

struct Enumerator : Named {
  ASTPtr<TypeName> value_type;

  Enumerator(Token tok) : Named(ASTKind::Enumerator, tok, tok) {
  }
};

struct Enum : Templatable {
  ASTVec<Enumerator> enumerators;

  Enum(Token tok, Token name)
      : Templatable(ASTKind::Enum, tok, name) {
  }
};

/*
  class MyClass {
    let num : int;

    fn new(n: int) -> MyClass {
      return MyClass{ num: n };
    }
  }
*/
struct Class : Templatable {
  ASTVec<VarDef> mb_variables;
  ASTVec<Function> mb_functions;

  Class(Token tok, Token name)
      : Templatable(ASTKind::Class, tok, name) {
  }
};

struct Program {
  ASTVector list;
};

} // namespace AST

} // namespace metro

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

  std::string_view trim(i64 len) {
    return std::string_view(this->source.data.data() + this->pos,
                            len);
  }

  SourceStorage const& source;
  i64 pos;
  i64 length;
};

} // namespace metro

namespace metro::parser {

using AST::ASTPointer;
using AST::ASTVector;

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

  ASTPointer ScopeResol();

  ASTPointer IndexRef();
  ASTPointer Unary();

  ASTPointer Mul();
  ASTPointer Add();
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