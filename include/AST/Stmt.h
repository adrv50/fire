#pragma once

namespace fire::AST {

struct Block : Base {
  ASTVector list;
  int stack_size = 0; // count of variable definition

  static ASTPtr<Block> New(Token tok, ASTVector list = {});

  ASTPointer Clone() const override;

  Block(Token tok, ASTVector list = {});
};

struct VarDef : Named {
  ASTPtr<TypeName> type;
  ASTPointer init;

  int index = 0;
  int index_add = 0;

  static ASTPtr<VarDef> New(Token tok, Token name, ASTPtr<TypeName> type,
                            ASTPointer init);

  static ASTPtr<VarDef> New(Token tok, Token name) {
    return New(tok, name, nullptr, nullptr);
  }

  ASTPointer Clone() const override;

  VarDef(Token tok, Token name, ASTPtr<TypeName> type = nullptr,
         ASTPointer init = nullptr);
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
    Vec<Case> cases;
  };

  struct While {
    ASTPointer cond;
    ASTPtr<Block> block;
  };

  struct TryCatch {
    struct Catcher {
      Token varname; // name of variable to catch exception instance
      ASTPtr<TypeName> type;

      ASTPtr<Block> catched;

      TypeInfo _type;
    };

    ASTPtr<Block> tryblock;
    Vec<Catcher> catchers;
  };

  union {
    If* data_if;
    Switch* data_switch;
    While* data_while;
    TryCatch* data_try_catch;

    void* _data = nullptr;
  };

  ASTPointer expr;

  static ASTPtr<Statement> NewIf(Token tok, ASTPointer cond, ASTPointer if_true,
                                 ASTPointer if_false = nullptr);

  static ASTPtr<Statement> NewSwitch(Token tok, ASTPointer cond,
                                     Vec<Switch::Case> cases = {});

  static ASTPtr<Statement> NewWhile(Token tok, ASTPointer cond, ASTPtr<Block> block);

  static ASTPtr<Statement> NewTryCatch(Token tok, ASTPtr<Block> tryblock,
                                       vector<TryCatch::Catcher> catchers);

  static ASTPtr<Statement> New(ASTKind kind, Token tok, void* data = nullptr);

  static ASTPtr<Statement> NewExpr(ASTKind kind, Token tok, ASTPointer expr);

  ASTPointer Clone() const override;

  Statement(ASTKind kind, Token tok, void* data);
  Statement(ASTKind kind, Token tok, ASTPointer expr);
  ~Statement();
};

} // namespace fire::AST