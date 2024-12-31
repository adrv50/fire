#include <iostream>

#include "alert.h"
#include "Lexer.h"

#include "Parser.h"
#include "Sema.h"
#include "Evaluator.h"

int main(int argc, char** argv) {
  try {
    (void)argc;
    (void)argv;

    SourceStorage SS{"test.fr"};

    Lexer lexer{SS};

    auto tok = lexer.lex();

    Parser parser{tok};

    auto prg = parser.parse();

    Sema sema{prg};

    sema.check_full();

    Evaluator ev{prg};

    ev.evaluate();
  }
  catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
    return -1;
  }
  catch (...) {
    return -1;
  }
}