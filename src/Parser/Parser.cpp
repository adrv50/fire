#include <iostream>

#include "alert.h"
#include "Lexer.h"
#include "Parser.h"
#include "Error.h"
#include "Utils.h"

namespace fire::parser {

ASTPointer Parser::Stmt() {

  auto& tok = *this->cur;

  if (this->eat("{")) {
    auto ast = AST::Block::New(tok);

    ast->token = tok;

    if (this->eat("}")) {
      ast->endtok = *this->ate;
      return ast;
    }

    while (this->check()) {
      ast->list.emplace_back(this->Stmt());

      if (this->eat("}")) {
        ast->endtok = *this->ate;
        return ast;
      }
    }

    throw Error(tok, "not terminated block");
  }

  if (this->eat("match")) {
    auto ast = AST::Match::New(tok, this->Expr(), {});

    this->expect("{");

    do {
      auto e = this->Expr();

      this->expect("=>");

      this->expect("{", true);

      auto& p = ast->patterns.emplace_back(AST::Match::Pattern::Type::Unknown, e,
                                           ASTCast<AST::Block>(this->Stmt()));

      if (e->IsUnqualifiedIdentifier() && e->token.str == "_") {
        p.everything = true;
      }

    } while (this->eat(","));

    this->expect("}");

    return ast;
  }

  if (this->eat("if")) {
    auto cond = this->Expr();

    this->expect("{", true);
    auto if_true = this->Stmt();

    ASTPointer if_false = nullptr;

    if (this->eat("else")) {
      if (!this->eat("if"))
        this->expect("{", true);

      if_false = this->Stmt();
    }

    return AST::Statement::NewIf(tok, cond, if_true, if_false);
  }

  if (this->eat("while")) {
    auto cond = this->Expr();

    this->expect("{", true);
    auto block = ASTCast<AST::Block>(this->Stmt());

    return AST::Statement::NewWhile(tok, cond, block);
  }

  if (this->eat("for")) {
    ASTPointer init = nullptr, cond = nullptr, step = nullptr;

    if (this->match("let")) {
      init = this->Stmt();
    }
    else if (!this->eat(";")) {
      init = this->Expr();
      this->expect(";");
    }

    if (!this->eat(";")) {
      cond = this->Expr();
      this->expect(";");
    }
    else {
      cond = AST::Value::New(*this->ate, ObjNew<ObjPrimitive>(true));
    }

    if (!this->match("{")) {
      step = this->Expr();
    }

    this->expect("{", true);
    auto block = ASTCast<AST::Block>(this->Stmt());

    return AST::Block::New(tok,
                           {init, AST::Statement::NewWhile(tok, cond,
                                                           AST::Block::New(tok, {
                                                                                    block,
                                                                                    step,
                                                                                }))});
  }

  if (this->eat("return")) {
    if (this->eat(";")) {
      return AST::Statement::New(ASTKind::Return, tok);
    }

    auto ast = AST::Statement::NewExpr(ASTKind::Return, tok, this->Expr());
    this->expect(";");

    return ast;
  }

  if (this->eat("break")) {
    if (!this->_in_loop)
      throw Error(tok, "cannot use 'break' out of loop statement");

    auto ast = AST::Statement::New(ASTKind::Break, tok);

    this->expect(";");
    return ast;
  }

  if (this->eat("continue")) {
    if (!this->_in_loop)
      throw Error(tok, "cannot use 'continue' out of loop statement");

    auto ast = AST::Statement::New(ASTKind::Continue, tok);

    this->expect(";");
    return ast;
  }

  if (this->eat("throw")) {
    auto ast = AST::Statement::NewExpr(ASTKind::Throw, tok, this->Expr());

    this->expect(";");
    return ast;
  }

  if (this->eat("let")) {
    auto ast = AST::VarDef::New(tok, *this->expectIdentifier());

    if (this->eat(":"))
      ast->type = this->expectTypeName();

    if (this->eat("="))
      ast->init = this->Expr();

    this->expect(";");
    return ast;
  }

  if (this->eat("try")) {
    this->expect("{", true);
    auto block = ASTCast<AST::Block>(this->Stmt());

    vector<AST::Statement::TryCatch::Catcher> catchers;

    while (this->eat("catch")) {
      auto name = *this->expectIdentifier();

      this->expect(":");
      auto type = this->expectTypeName();

      this->expect("{", true);
      auto block = ASTCast<AST::Block>(this->Stmt());

      catchers.push_back({name, type, block, {}});
    }

    return AST::Statement::NewTryCatch(tok, block, std::move(catchers));
  }

  auto ast = this->Expr();

  this->expect(";");

  return ast;
}

ASTPointer Parser::Top() {

  auto tok = *this->cur;
  auto iter = this->cur;

  if (this->eat("enum")) {
    auto ast = AST::Enum::New(tok, *this->expectIdentifier());

    this->expect("{");

    if (this->eat("}")) {
      Error(*this->ate, "empty enum is not valid").emit();

      return ast;
    }

    do {
      auto& e = ast->append(*this->expectIdentifier());

      if (this->eat("(")) {
        if (this->match(TokenKind::Identifier, ":")) {
          e.data_type = AST::Enum::Enumerator::DataType::Structure;

          do {
            auto& name = *this->expectIdentifier();

            this->cur++; // ":"
            auto type = this->expectTypeName();

            e.types.emplace_back(AST::Argument::New(name, type));
          } while (this->eat(","));
        }
        else {
          e.data_type = AST::Enum::Enumerator::DataType::Value;
          e.types.emplace_back(this->expectTypeName());
        }

        this->expect(")");
      }
    } while (this->eat(","));

    this->expect("}");

    return ast;
  }

  bool ate_final_tok = false;

  // final class
  if (this->eat("final")) {
    this->expect("class", true);
    ate_final_tok = true;
  }

  if (this->eat("class")) {
    auto ast = AST::Class::New(tok, *this->expectIdentifier());

    ast->IsFinal = ate_final_tok;
    ast->FianlSpecifyToken = tok;

    if (this->eat("extends")) {
      ast->InheritBaseClassName = this->ScopeResol();
    }

    //
    // todo: inherit interfaces
    //

    this->expect("{");

    bool closed = false;

    auto s1 = this->_in_class;
    auto s2 = this->_classptr;

    this->_in_class = true;
    this->_classptr = ast;

    // member variables
    while (this->cur->str == "let") {
      auto& var = ast->append_var(ASTCast<AST::VarDef>(this->Stmt()));

      if (!var->type && !var->init) {
        throw Error(var->token, "cannot use delay-assignment here");
      }
    }

    // member functions
    while (this->check() && !(closed = this->eat("}"))) {
      if (!this->match("virtual") && !this->match("fn", TokenKind::Identifier))
        throw Error(*this->cur, "expected definition of member function");

      ast->append_func(ASTCast<AST::Function>(this->Top()));
    }

    if (!closed)
      throw Error(iter[2], "not terminated block");

    this->_in_class = s1;
    this->_classptr = s2;

    return ast;
  }

  bool _f_virtualized = false;
  TokenIterator virtual_tok = this->cur;

  if (this->eat("virtual")) {
    if (!this->_in_class) {
      throw Error(*this->ate, "cannot define virtualized function out of class");
    }

    this->expect("fn", true);

    _f_virtualized = true;
  }

  if (this->eat("fn")) {

    auto func = AST::Function::New(tok, *this->expectIdentifier());

    if ((func->is_virtualized = _f_virtualized)) {
      func->virtualize_specify_tok = *virtual_tok;
    }

    if (this->eat_typeparam_bracket_open()) {
      func->IsTemplated = true;
      func->TemplateTok = *this->cur;

      do {
        func->ParameterList.emplace_back(this->parse_template_param_decl());
      } while (this->eat(","));

      this->expect_typeparam_bracket_close();
    }

    this->expect("(");

    if (auto tokkk = this->cur; this->_in_class && this->eat("self")) {
      func->member_of = this->_classptr;

      if (!this->eat(","))
        this->expect(")", true);

      func->add_arg(*tokkk, AST::TypeName::New(this->_classptr->name));
    }

    if (!this->eat(")")) {
      do {
        auto& arg = func->add_arg(*this->expectIdentifier());

        this->expect(":");
        arg->type = this->expectTypeName();
      } while (this->eat(","));

      this->expect(")");
    }

    if (this->eat("->")) {
      func->return_type = this->expectTypeName();
    }

    if (this->eat("override")) {
      if (!this->_in_class) {
        throw Error(*this->ate, "cannot define overrided function at out of scope");
      }

      func->is_override = true;
      func->override_specify_tok = *this->ate;
    }

    this->expect("{", true);
    func->block = ASTCast<AST::Block>(this->Stmt());

    return func;
  }

  if (this->eat("namespace")) {
    auto ast = AST::Block::New(*this->expectIdentifier());

    ast->kind = ASTKind::Namespace;

    iter = this->cur;
    this->expect("{");

    if (this->eat("}"))
      return ast;

    bool closed = false;

    do {
      ast->list.emplace_back(this->Top());
    } while (this->check() && !(closed = this->eat("}")));

    if (!closed)
      throw Error(*iter, "unterminated namespace block");

    return ast;
  }

  return this->Stmt();
}

ASTPtr<AST::Block> Parser::Parse() {
  auto ret = AST::Block::New(*this->cur);

  while (this->eat("include")) {
    auto tok = this->ate;

    if (!this->check() || this->cur->kind != TokenKind::String) {
      throw Error(*tok, "expected string literal after this token");
    }

    auto path = string(this->cur->str);

    this->cur++;

    path.erase(path.begin());
    path.pop_back();

    auto& src = tok->sourceloc.ref->AddIncluded(path);

    if (!src.Open()) {
      throw Error(*tok, "cannot open file '" + src.path + "'");
    }

    Lexer lexer{src};

    lexer.Lex(src.token_list);

    Parser parser{src.token_list};

    auto parsed = parser.Parse();

    for (auto&& e : parsed->list)
      ret->list.emplace_back(e);

    this->expect(";");
  }

  while (this->check()) {
    ret->list.emplace_back(this->Top());
  }

  for (size_t i = 0; i < this->tokens.size(); i++) {
    this->tokens[i]._index = (i64)i;
  }

  return ret;
}

Parser::Parser(TokenVector& tokens)
    : tokens(tokens),
      cur(this->tokens.begin()),
      end(this->tokens.end()) {
}

} // namespace fire::parser