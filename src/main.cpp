#include <iostream>

#include "alert.h"
#include "Utils/Utils.h"
#include "Parser/Lexer.h"
#include "Parser/Parser.h"

int main(int argc, char** argv) {
  using namespace metro;
  using namespace metro::parser;

  SourceStorage source{"test.metro"};

  if (!source.Open())
    return 1;

  Lexer lexer{source};

  parser::Parser parser{lexer.Lex()};

  auto ast = parser.Parse();

  return 0;
}