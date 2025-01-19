#include "SourceStorage.h"
#include "Evaluator.h"
#include "Driver/Driver.h"
#include "alert.h"

static constexpr auto help_string = R"(
usage: fire [options...] ...

options:
  -h   --help        show this message
  -v   --version     show version info
)";

static constexpr auto version_string = R"(
fire 0.0.1b
Copyright (C) 2024 Aoki.
)";

namespace fire {

Driver::Driver() {
}

Driver::~Driver() {
}

int Driver::main(int argc, char** argv) {
}

Obj Driver::execute(SourceStorage const& source) {
  return Evaluator(source.get_parsed()).evaluate();
}

void Driver::add_error(Error&& e) {
  Driver::get_instance()->errors.emplace_back(std::move(e));
}

CmdOptions Driver::parse_arguments(int argc, char** argv) {
  Vec<string> args;

  CmdOptions opt;

  while (argc--)
    args.emplace_back(*argv++);

  for (auto it = args.begin(); it != args.end();) {
    if (*it == "-h" || *it == "--help") {
    }

    else {
      opt.run_files.emplace_back(*it++);
    }
  }

  return opt;
}

} // namespace fire