#pragma once

#include <any>
#include <algorithm>
#include <concepts>
#include <string>
#include <fstream>

#include "Object.h"

namespace fire {

struct SourceStorage {
  struct LineRange {
    i64 index, begin, end, length;

    LineRange(i64 index, i64 begin, i64 end)
        : index(index),
          begin(begin),
          end(end),
          length(end - begin) {
    }

    LineRange()
        : LineRange(0, 0, 0) {
    }
  };

  std::string path;
  std::string data;

  std::unique_ptr<std::ifstream> file;

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
      : position(pos),
        length(len),
        pos_in_line(0),
        ref(r) {
    if (this->ref) {
      this->line = this->ref->GetLineRange(this->position);
      this->pos_in_line = this->position - this->line.begin;
    }
  }

  SourceLocation()
      : SourceLocation(0, 0, nullptr) {
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
      : kind(kind),
        str(str),
        sourceloc(sourceloc) {
  }

  Token(std::string str)
      : Token(TokenKind::Unknown, str) {
  }

  Token(char const* str = "")
      : Token(std::string(str)) {
  }

  Token(TokenKind kind)
      : Token(kind, "") {
  }
};

enum class ASTKind {
  Value,
  Variable,

  IndexRef,
  MemberAccess,
  CallFunc,

  SpecifyArgumentName, // func(a: 2, b: "aiue")

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

  Break,
  Continue,
  Return,

  // FuncArg,
  Function,

  Enumerator,
  Enum,

  Class,

  Module,

  TypeName,

  Program,
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
    return static_cast<T const*>(this);
  }

  i64 GetChilds(ASTVector& out) const;

  virtual std::string_view GetSourceView() const {
    return token.sourceloc.GetLine();
  }

  virtual ~Base() = default;

protected:
  Base(ASTKind kind, Token token)
      : kind(kind),
        token(token),
        endtok(token) {
  }

  Base(ASTKind kind, Token token, Token endtok)
      : kind(kind),
        token(token),
        endtok(endtok) {
  }
};

struct Value : Base {
  ObjPointer value;

  static ASTPtr<Value> New(Token tok, ObjPointer val);

  Value(Token tok, ObjPointer value)
      : Base(ASTKind::Value, tok),
        value(value) {
  }
};

struct Named : Base {
  Token name;

  std::string GetName() const {
    return this->name.str;
  }

protected:
  Named(ASTKind kind, Token tok, Token name)
      : Base(kind, tok),
        name(name) {
    this->is_named = true;
  }

  Named(ASTKind kind, Token tok)
      : Named(kind, tok, tok) {
  }
};

struct Variable : Named {
  static ASTPtr<Variable> New(Token tok);

  Variable(Token tok)
      : Named(ASTKind::Variable, tok, tok) {
  }
};

struct CallFunc : Base {
  ASTPointer expr; // left side
  ASTVector args;

  static ASTPtr<CallFunc> New(ASTPointer expr, ASTVector args = {});

  CallFunc(ASTPointer expr, ASTVector args = {})
      : Base(ASTKind::CallFunc, expr->token, expr->token),
        expr(expr),
        args(std::move(args)) {
  }
};

struct Expr : Base {
  Token op;
  ASTPointer lhs;
  ASTPointer rhs;

  static ASTPtr<Expr> New(ASTKind kind, Token optok, ASTPointer lhs,
                          ASTPointer rhs);

  Expr(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs)
      : Base(kind, optok),
        op(this->token),
        lhs(lhs),
        rhs(rhs) {
    this->is_expr = true;
  }
};

struct Block : Base {
  ASTVector list;

  static ASTPtr<Block> New(Token tok, ASTVector list = {});

  Block(Token tok /* "{" */, ASTVector list = {})
      : Base(ASTKind::Block, tok),
        list(std::move(list)) {
  }
};

struct VarDef : Named {
  ASTPtr<TypeName> type;
  ASTPointer init;

  static ASTPtr<VarDef> New(Token tok, Token name,
                            ASTPtr<TypeName> type, ASTPointer init);

  static ASTPtr<VarDef> New(Token tok, Token name) {
    return New(tok, name, nullptr, nullptr);
  }

  VarDef(Token tok, Token name, ASTPtr<TypeName> type = nullptr,
         ASTPointer init = nullptr)
      : Named(ASTKind::Vardef, tok, name),
        type(type),
        init(init) {
  }
};

struct Statement : Base {
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
    ASTVector init;
    ASTPointer cond;
    ASTVector count;
    ASTPtr<Block> block;
  };

  //
  // astdata
  //
  // ASTKind::Return
  //  --> astdata = ASTPtr<AST::Expr>
  //
  std::any astdata;

  static ASTPtr<Statement> NewIf(Token tok, ASTPointer cond,
                                 ASTPointer if_true,
                                 ASTPointer if_false = nullptr);

  static ASTPtr<Statement>
  NewSwitch(Token tok, ASTPointer cond,
            std::vector<Switch::Case> cases = {});

  static ASTPtr<Statement> NewFor(Token tok, ASTVector init,
                                  ASTPointer cond, ASTVector count,
                                  ASTPtr<Block> block);

  static ASTPtr<Statement> New(ASTKind kind, Token tok,
                               std::any data = (ASTPointer) nullptr);

  Statement(ASTKind kind, Token tok, std::any data)
      : Base(kind, tok),
        astdata(data) {
  }
};

struct TypeName : Named {
  ASTVector type_params;
  ASTVec<TypeName> scope_resol;
  bool is_const;

  TypeInfo type;
  ASTPtr<Class> ast_class;

  static ASTPtr<TypeName> New(Token nametok);

  TypeName(Token name)
      : Named(ASTKind::TypeName, name),
        is_const(false) {
  }
};

struct Function : Named {
  TokenVector arg_names;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  bool is_var_arg;

  Token& add_arg(Token const& tok) {
    return this->arg_names.emplace_back(tok);
  }

  static ASTPtr<Function> New(Token tok, Token name);

  static ASTPtr<Function> New(Token tok, Token name,
                              TokenVector arg_names, bool is_var_arg,
                              ASTPtr<TypeName> rettype,
                              ASTPtr<Block> block);

  Function(Token tok, Token name)
      : Named(ASTKind::Function, tok, name),
        return_type(nullptr),
        block(nullptr),
        is_var_arg(false) {
  }

  Function(Token tok, Token name, TokenVector args, bool is_var_arg,
           ASTPtr<TypeName> rettype, ASTPtr<Block> block)
      : Named(ASTKind::Function, tok, name),
        arg_names(args),
        return_type(rettype),
        block(block),
        is_var_arg(is_var_arg) {
  }
};

struct Enum : Named {
  TokenVector enumerators;

  static ASTPtr<Enum> New(Token tok, Token name);

  Enum(Token tok, Token name)
      : Named(ASTKind::Enum, tok, name) {
  }
};

struct Class : Named {
  ASTVec<VarDef> mb_variables;
  ASTVec<Function> mb_functions;

  ASTPtr<Function> constructor = nullptr;

  static ASTPtr<Class> New(Token tok, Token name);

  static ASTPtr<Class> New(Token tok, Token name,
                           ASTVec<VarDef> var_decl_list,
                           ASTVec<Function> func_list,
                           ASTPtr<Function> ctor);

  Class(Token tok, Token name)
      : Named(ASTKind::Class, tok, name) {
  }
};

struct Program : Base {
  ASTVector list;

  static ASTPtr<Program> New();

  Program()
      : Base(ASTKind::Program, {}) {
  }
};

} // namespace AST

} // namespace fire
