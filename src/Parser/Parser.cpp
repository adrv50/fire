#include <iostream>

#include "alert.h"
#include "Parser.h"
#include "Error.h"
#include "Utils.h"

namespace fire::parser {

ASTPointer Parser::Stmt() {

  auto tok = *this->cur;

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

    auto ast = AST::Statement::New(ASTKind::Return, tok, this->Expr());
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
    auto ast = AST::Statement::New(ASTKind::Throw, tok, this->Expr());

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
      ast->append(*this->expectIdentifier());
    } while (this->eat(","));

    this->expect("}");

    return ast;
  }

  if (this->eat("class") || this->eat("struct")) {
    auto ast = AST::Class::New(tok, *this->expectIdentifier());

    this->expect("{");

    bool closed = false;

    auto s1 = this->_in_class;
    auto s2 = this->_classptr;

    this->_in_class = true;
    this->_classptr = ast;

    // member variables
    while (this->cur->str == "let") {
      ast->append(ASTCast<AST::VarDef>(this->Stmt()));
    }

    // constructor
    if (this->eat(ast->GetName())) {

      auto ctor = AST::Function::New(ast->name, ast->name);

      this->expect("(");

      this->expect("self");
      ctor->add_arg(*this->ate);

      while (this->eat(",")) {
        ctor->add_arg(*this->expectIdentifier());
      }

      this->expect(")");

      this->expect("{", true);
      ctor->block = ASTCast<AST::Block>(this->Stmt());

      ast->constructor = ASTCast<AST::Function>(ast->append(ctor));
    }

    // member functions
    while (this->check() && !(closed = this->eat("}"))) {
      if (!this->match("fn", TokenKind::Identifier))
        throw Error(*this->cur, "expected definition of member function");

      ast->append(ASTCast<AST::Function>(this->Top()));
    }

    if (!closed)
      throw Error(iter[2], "not terminated block");

    this->_in_class = s1;
    this->_classptr = s2;

    return ast;
  }

  if (this->eat("fn")) {
    auto func = AST::Function::New(tok, *this->expectIdentifier());

    if (this->eat_typeparam_bracket_open()) {
      func->is_templated = true;

      do {
        func->template_param_names.emplace_back(*this->expectIdentifier());
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

    this->expect("{", true);
    func->block = ASTCast<AST::Block>(this->Stmt());

    return func;
  }

  //
  // import <name>
  //   --> convert to " let name = import("name"); "
  //
  // replace to variable declaration,
  //  and assignment result of call "import" func.
  //
  if (this->match("import", TokenKind::Identifier) || this->match("import", ".")) {

    // import a;
    // import ./b;
    // import ../c;

    this->cur++;
    std::string name;

    if (this->eat(".")) {
      name += ".";

      if (this->eat("."))
        name += ".";

      this->expect("/");
      name += "/";
    }

    name = this->expectIdentifier()->str;

    while (this->eat("/")) {
      name += "/" + this->expectIdentifier()->str;
    }

    name = name + ".fire";

    this->expect(";");

    auto ast = AST::VarDef::New(tok, utils::get_base_name(name));

    auto call = AST::CallFunc::New(AST::Variable::New("@import"));

    Token mod_name_token = iter[1]; // for argument of @import

    mod_name_token.kind = TokenKind::String;
    mod_name_token.str = name;
    mod_name_token.sourceloc.length = name.length();

    call->args.emplace_back(AST::Value::New(mod_name_token, ObjNew<ObjString>(name)));

    ast->init = call;

    return ast;
  }

  return this->Stmt();
}

ASTPtr<AST::Block> Parser::Parse() {
  auto ret = AST::Block::New("");

  while (this->check()) {
    ret->list.emplace_back(this->Top());
  }

  return ret;
}

Parser::Parser(TokenVector tokens)
    : tokens(std::move(tokens)),
      cur(this->tokens.begin()),
      end(this->tokens.end()) {
}

} // namespace fire::parser