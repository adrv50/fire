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

#include "Object.h"
#include "Token.h"
#include "Node.h"

#include "Utils.h"

#include "node2s.h"

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

    debug(std::cout << node2s(prg) << std::endl;);

    sema::Sema S{prg};

    S.check_full();

    Evaluator ev{prg};

    ev.evaluate();

    return 0;
  }

  catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
    return -1;
  }

  catch (Error const& e) {
    e.emit();
  }

  return 1;
}