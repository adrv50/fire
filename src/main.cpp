#include <iostream>
#include "alert.h"
#include "Utils.h"
#include "Parser.h"
#include "Checker.h"
#include "Evaluator.h"

#include "AST.h"

namespace metro::sema {

class Sema {

  struct TypeContext {
    Sema& S;
    ASTPointer ast;
    ASTVector candidates;

    TypeContext(Sema& S, ASTPointer ast)
        : S(S),
          ast(ast) {
    }
  };

public:
  Sema(ASTPtr<AST::Program> prg)
      : root(prg) {
  }

  void do_check() {
    for (auto&& x : this->root->list)
      this->walk(x);
  }

  void walk(ASTPointer ast) {

    switch (ast->kind) {}
  }

private:
  ASTPtr<AST::Program> root;
};

} // namespace metro::sema

int main(int argc, char** argv) {
  using namespace metro;
  using namespace metro::parser;

  SourceStorage source{"test.metro"};

  if (!source.Open())
    return 1;

  Lexer lexer{source};

  parser::Parser parser{lexer.Lex()};
  auto prg = parser.Parse();

  alert;

  sema::Sema checker{prg};

  checker.do_check();

  // checker::Checker chk{prg};
  // chk.do_check();

  // eval::Evaluator eval{prg};
  // eval.do_eval();

  return 0;
}