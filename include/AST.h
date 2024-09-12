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

    LineRange(i64 index, i64 begin, i64 end)
        : index(index), begin(begin), end(end), length(end - begin) {
    }

    LineRange() : LineRange(0, 0, 0) {
    }
  };

  std::string path;
  std::string data;

  PUnique<std::ifstream> file;

  std::vector<LineRange> line_range_list;

  bool Open();
  bool IsOpen() const;

  LineRange GetLineRange(i64 position) const;
  std::string_view GetLineView(LineRange const& line) const;
  std::vector<LineRange> GetLinesOfAST(ASTPointer ast);

  std::string_view GetLineView(i64 index) const {
    return this->GetLineView(this->line_range_list[index]);
  }

  int Count() const {
    return this->line_range_list.size();
  }

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
    return this->ref->GetLineView(this->line);
  }

  SourceLocation(i64 pos, i64 len, SourceStorage const* r)
      : position(pos), length(len), pos_in_line(0), ref(r) {
    if (this->ref) {
      this->line = this->ref->GetLineRange(this->position);
      this->pos_in_line = this->position - this->line.begin;
    }
  }

  SourceLocation() : SourceLocation(0, 0, nullptr) {
  }
};

enum class TokenKind : u8 {
  Unknown,
  Int,
  Hex,
  Bin,
  Float,
  Size,
  Boolean,
  Char,
  String,
  Identifier,
  Keyword,
  Punctuater,
};

struct Token {
  TokenKind kind;
  std::string str;
  SourceLocation sourceloc;

  bool operator==(Token const& tok) const {
    return this->kind == tok.kind && this->str == tok.str;
  }

  Token(TokenKind kind, std::string const& str,
        SourceLocation sourceloc = {})
      : kind(kind), str(str), sourceloc(sourceloc) {
  }

  Token(std::string str) : Token(TokenKind::Unknown, str) {
  }

  Token(char const* str = "") : Token(std::string(str)) {
  }

  Token(TokenKind kind) : Token(kind, "") {
  }
};

enum class ASTKind {
  Value,
  Variable,

  IndexRef,
  MemberAccess,
  CallFunc,

  Not, // !

  Mul,
  Div,
  Add,
  Sub,
  LShift,
  RShift,
  Bigger,        // a > b
  BiggerOrEqual, // a >= b
  Equal,

  BitAND,
  BitXOR,
  BitOR,

  LogAND,
  LogOR,

  Assign,

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

  TypeName,
};

namespace AST {

struct Base {
  ASTKind kind;
  Token token;
  Token endtok;

  bool is_named = false;
  bool is_expr = false;

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
  Base(ASTKind kind, Token token, Token endtok)
      : kind(kind), token(token), endtok(endtok) {
  }

  Base(ASTKind kind, Token token) : Base(kind, token, token) {
  }
};

struct Value : Base {
  ObjPointer value;

  Value(Token tok, ObjPointer value)
      : Base(ASTKind::Value, tok), value(value) {
  }
};

struct Named : Base {
  Token name;

  std::string GetName() const {
    return this->name.str;
  }

protected:
  Named(ASTKind kind, Token tok, Token name)
      : Base(kind, tok), name(name) {
    this->is_named = true;
  }

  Named(ASTKind kind, Token tok) : Named(kind, tok, tok) {
  }
};

struct Variable : Named {
  Variable(Token tok) : Named(ASTKind::Variable, tok, tok) {
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

  Expr(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs)
      : Base(kind, optok), op(this->token), lhs(lhs), rhs(rhs) {
    this->is_expr = true;
  }
};

struct Block : Base {
  ASTVector list;

  Block(Token tok, ASTVector list = {}) : Base(ASTKind::Block, tok) {
  }
};

struct VarDef : Named {
  ASTPtr<TypeName> type = nullptr;
  ASTPointer init = nullptr;

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
  builtins::Class const* builtin_class;

  TypeName(Token name)
      : Base(ASTKind::TypeName, name), name(this->token),
        is_const(false), builtin_class(nullptr) {
  }
};

struct FuncArgument : Named {
  ASTPtr<TypeName> type;

  FuncArgument(Token tok, ASTPtr<TypeName> type)
      : Named(ASTKind::FuncArg, tok, tok), type(type) {
  }
};

struct Function : Named {
  ASTVec<FuncArgument> args;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  Function(Token tok, Token name)
      : Named(ASTKind::Function, tok, name) {
  }
};

struct Enumerator : Named {
  ASTPtr<TypeName> value_type;

  Enumerator(Token tok) : Named(ASTKind::Enumerator, tok, tok) {
  }
};

struct Enum : Named {
  ASTVec<Enumerator> enumerators;

  Enum(Token tok, Token name) : Named(ASTKind::Enum, tok, name) {
  }
};

struct Class : Named {
  ASTVec<VarDef> mb_variables;
  ASTVec<Function> mb_functions;

  Class(Token tok, Token name) : Named(ASTKind::Class, tok, name) {
  }
};

struct Program {
  ASTVector list;
};

} // namespace AST

} // namespace metro
