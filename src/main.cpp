#include <iostream>

#include "alert.h"
#include "fire-fwd.h"
#include "SourceStorage.h"

#include "Repl.h"
#include "Lexer.h"
#include "Parser.h"
// #include "Sema.h"
#include "Evaluator.h"

#include "Sema/Sema.h"

#include "Error.h"

#include "Object.h"
#include "Token/Token.h"
#include "Node/Node.h"

#include "Utils.h"

#include "Node/node2s.h"

#include "Builtins.h"

using namespace fire;

int main(int argc, char** argv) {
  Builtins::initialize();

  Vec<SourceStorage> sources;

  SourceStorage SS;

  try {
    (void)argv;

    if (argc == 1) {
      Repl::run();
      return 0;
    }

    SS.open("test.fr");
    SS.read();

    Lexer lexer{SS};

    auto tok = lexer.lex();

    if (tok->is(TokenKind::End)) // empty source file
      return 0;

    Parser parser{tok};

    auto prg = parser.parse();

    debug std::cout << node2s(prg) << std::endl;

    fire::sema::Sema S{prg};

    S.check_all();

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