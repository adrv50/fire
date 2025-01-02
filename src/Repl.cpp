#include <iostream>
#include <fstream>

#include "alert.h"

#include "Token.h"
#include "Node.h"
#include "Object.h"

#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "Evaluator.h"
#include "Repl.h"

#include "Error.h"

using std::cin;
using std::cout;
using std::endl;

using std::getline;

using Kwd = TokenKwdKind;
using Op = TokenOperatorKind;
using Punct = TokenPunctKind;

namespace Repl {

Node* prg;

unique_ptr<Parser> parser;

unique_ptr<Sema> sema;

unique_ptr<Evaluator> ev;

void init() {
  prg = Node::new_node(ND_Program, nullptr);
}

string input_line(string const& prompt) {
  cout << prompt;

  string line;

  getline(cin, line);

  return line;
}

Token* lex_line(string const& line) {
  (void)line;

  return nullptr;
}

Obj execute_line(string const& line) {
  (void)line;

  return nullptr;
}

void run() {
  init();

  cout << "Fire 0.0.1 on linux" << endl;
  cout << "type 'help', 'license' for more information." << endl;
  cout << "type 'exit' to exit." << endl;

  while (true) {
    auto line = input_line(">>> ");

    if (line.empty())
      continue;

    if (line == "exit")
      break;

    if (line == "help") {
      cout << "help(todo)" << endl;
      continue;
    }

    if (line == "license") {
      cout << "license(todo)" << endl;
      continue;
    }

    auto tok = lex_line(line);

    (void)tok;

    if (auto result = execute_line(line))
      cout << result->to_string() << endl;
  }
}

} // namespace Repl
