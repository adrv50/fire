#include <iostream>

#include "alert.h"

#include "AST.h"
#include "Utils.h"
#include "Error.h"

#include "Parser.h"
#include "Checker.h"
#include "Evaluator.h"

static constexpr auto command_help = R"(
usage: flame [options] scripts...

options:
    -h --help         show this information
    -v --version      show version info
)";

static constexpr auto command_version = R"(
flame 0.0.1
)";

struct CmdLineArguments {

  // -h, --help
  bool help = false;

  // -v, --version
  bool version_info = false;

  //
  // [source files]
  StringVector sources;
};

int parse_command_line(CmdLineArguments& cmd, int argc, char** argv) {

  while (argc--) {
    std::string arg = *argv++;

    if (arg == "-h" || arg == "--help")
      cmd.help = true;

    else if (arg == "-v" || arg == "--version")
      cmd.version_info = true;

    else
      cmd.sources.emplace_back(std::move(arg));
  }

  return 0;
}

int main(int argc, char** argv) {
  using namespace metro;
  using namespace metro::parser;

  CmdLineArguments args;

  parse_command_line(args, argc - 1, argv + 1);

  if (args.help) {
    std::cout << command_help << std::endl;
    return 0;
  }

  else if (args.version_info) {
    std::cout << command_version << std::endl;
    return 0;
  }

  if (args.sources.empty()) {
    Error::fatal_error("no input files.");
  }

  for (auto&& path : args.sources) {
    SourceStorage source{path};

    if (!source.Open()) {
      Error::fatal_error("cannot open file '" + path + "'");
    }

    Lexer lexer{source};

    parser::Parser parser{lexer.Lex()};
    auto prg = parser.Parse();

    eval::Evaluator eval{prg};
    eval.do_eval();
  }

  return 0;
}