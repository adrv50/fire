#pragma once

#include <any>
#include <algorithm>
#include <concepts>
#include <string>
#include <fstream>

#include "Object.h"

namespace metro {

struct SourceStorage {
  struct LineRange {
    i64 index, begin, end, length;
  };

  std::string path;
  std::string data;

  std::unique_ptr<std::ifstream> file;

  std::vector<LineRange> line_range_list;

  LineRange GetLineRange(i64 position) const;
  std::string_view GetLineView(LineRange const& line) const;
  std::vector<LineRange> GetLinesOfAST(AST::ASTPointer ast);

  bool Open();
  bool IsOpen() const;

  char operator[](size_t N) const {
    return this->data[N];
  }

  SourceStorage(std::string path);
};

struct SourceLocation {
  i64 position;
  i64 length;
  i64 pos_in_line;

  SourceStorage const* ref;
  SourceStorage::LineRange line;

  std::string_view GetLine() const {
    return ref->GetLineView(line);
  }

  SourceLocation(i64 pos, i64 len, SourceStorage const* ref)
      : position(pos), length(len), pos_in_line(0), ref(ref) {
    if (ref) {
      this->line = ref->GetLineRange(this->position);
      this->pos_in_line = this->position - line.begin;
    }
  }

  SourceLocation() : SourceLocation(0, 0, nullptr) {
  }
};

enum class TokenKind : u8 {
  Unknown,
  Int,
  Float,
  Size,
  Boolean,
  Char,
  String,
  Identifier,
  Punctuater,
};

struct Token {
  TokenKind kind;
  std::string_view str;
  SourceLocation sourceloc;

  Token(TokenKind kind, std::string_view str,
        SourceLocation sourceloc = {})
      : kind(kind), str(str), sourceloc(std::move(sourceloc)) {
  }

  Token(std::string_view str) : Token(TokenKind::Unknown, str) {
  }

  Token(char const* str = "") : Token(TokenKind::Unknown, str) {
  }

  Token(TokenKind kind) : Token(kind, "") {
  }
};

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
