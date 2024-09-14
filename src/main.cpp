#include <iostream>
#include "alert.h"
#include "Utils.h"
#include "Parser.h"
#include "Checker.h"
#include "Evaluator.h"

#include "AST.h"

int main(int argc, char** argv) {
  using namespace metro;
  using namespace metro::parser;

  SourceStorage source{"test.metro"};

  if (!source.Open())
    return 1;

  Lexer lexer{source};

  parser::Parser parser{lexer.Lex()};
  auto prg = parser.Parse();

  eval::Evaluator eval{prg};
  eval.do_eval();

  return 0;
}