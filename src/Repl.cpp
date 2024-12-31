#include <iostream>
#include <fstream>

#include "alert.h"

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

void run() {
  cout << "Fire 0.0.1 on linux" << endl;
  cout << "type 'help', 'license' for more information." << endl;
  cout << "type 'exit' to exit." << endl;

  Node* prg = Node::new_node(ND_Program, nullptr);

  Evaluator ev{prg};

  Sema sema{prg};

  auto fn_main = Node::new_node(
      ND_Function, Token::make(TokenKind::Identifier, nullptr, nullptr, "fn", 0));

  fn_main->tok->set_kwd(TokenKwdKind::Func);

  fn_main->nd_func_name = Token::make(TokenKind::Identifier, nullptr, nullptr, "main", 0);

  fn_main->nd_func_body = Node::new_node(ND_Block, nullptr);

  auto mainblock = fn_main->nd_func_body;

  (void)mainblock;

  while (true) {
    cout << ">>> ";

    string line;

    getline(cin, line);

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

    auto ss = SourceStorage::from_line(line + "\n");

    std::unique_ptr<SourceStorage> block_ss;

    Token* tok = Lexer(ss).lex();

    Token* last = tok;

    while (last->next->kind != TokenKind::End)
      last = last->next;

    if (last->is_punct(Punct::BlockBraceOpen)) {
      string block;
      string line2;

      do {
        cout << "    ";
        getline(cin, line2);
        block += line2 + "\n";
      } while (!line2.empty());

      block_ss.reset(new SourceStorage(SourceStorage::from_line(block)));

      last->next = Lexer(*block_ss).lex();
    }

    Parser parser(tok);

    parser.set_in_repl();

    auto node = parser.p_root();

    switch (node->kind) {
    case ND_Let:
      ev.eval_let(node);
      break;

    case ND_Function:
      todo_impl;
      break;

    default:
      if (auto result = ev.eval_expr(node); result && result != Object::none)
        cout << result->to_string() << endl;

      break;
    }
  }
}

} // namespace Repl
