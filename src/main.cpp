#include <iostream>

#include "alert.h"
#include "fire-fwd.h"
#include "SourceStorage.h"

#include "Repl.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "Evaluator.h"

#include "Error.h"

int main(int argc, char** argv) {
  SourceStorage SS{"test.fr"};

  try {
    (void)argv;

    if (argc == 1) {
      Repl::run();
      return 0;
    }

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

  catch (Error const& e) {
    e.emit();
  }

  catch (...) {
    return -1;
  }
}