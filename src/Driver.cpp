#include <iostream>

#include "alert.h"

#include "AST.h"
#include "Utils.h"
#include "Error.h"

#include "Lexer.h"
#include "Parser.h"
#include "Sema/Sema.h"
#include "Evaluator.h"

#include "Driver.h"

namespace fire {

static constexpr auto command_help = R"(
usage: flame [options] scripts...

options:
    -h --help         show this information
    -v --version      show version info
)";

static constexpr auto command_version = R"(
fire 0.0.1
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

FireDriver::FireDriver() {
}

FireDriver::~FireDriver() {
}

int FireDriver::fire_main(int argc, char** argv) {
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
    fire::Error::fatal_error("no input files.");
  }

  for (auto&& path : args.sources) {
    sources.emplace_back(path);
  }

  // execute all files!
  for (auto&& S : this->sources) {
    this->execute(S);
  }

  return 0;
}

ObjPointer FireDriver::execute(SourceStorage& source) {

  try {

    if (!source.Open()) {
      Error::fatal_error("cannot open file '" + source.path + "'");
    }

    Lexer lexer{source};

    lexer.Lex(source.token_list);

    if (source.token_list.empty()) {
      return nullptr;
    }

    parser::Parser parser{source.token_list};

    ASTPtr<AST::Block> prg = parser.Parse();

    semantics_checker::Sema sema{prg};

    sema.check_full();

    eval::Evaluator ev;

    return ev.evaluate(prg);
  }

  catch (Error const& err) {
    err.emit();
  }

  catch (ObjPointer obj) {
    Error::fatal_error("throwed unhandled exception object of '" + obj->type.to_string() +
                       "'");
  }

  return nullptr;
}

} // namespace fire