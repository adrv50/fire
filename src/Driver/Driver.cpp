
#include <iostream>

#include "Debug/Debug.h"
#include "Builtins.h"
#include "Node/Node.h"
#include "SourceStorage.h"
#include "Evaluator.h"
#include "Repl.h"
#include "Driver/Driver.h"

#include "Sema/Sema.h"

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

using std::cout;
using std::endl;

static Driver* g_instance;

static void test1() {

  using namespace Debug;

  auto node = make_nd_root(
      {make_nd_func("func", {{"a", make_nd_type("int")}}, make_nd_block({}))});

  try {
    sema::Sema(node).check_all();
  }
  catch (Error const& e) {
    cout << "in test func(): Error: " << e.get_message() << endl;
  }

  std::exit(0);
}

Driver::Driver() {
  Builtins::initialize();
}

Driver::~Driver() {
}

int Driver::main(int argc, char** argv) {

  this->opt = parse_arguments(argc, argv);

  test1();

  if (this->opt.run_repl) {
    Repl::run();
  }

  for (auto&& path : opt.run_files) {
    try {
      this->execute(this->add_source(path));
    }
    catch (Error const& e) {
      e.emit();
    }
  }

  return 0;
}

Obj Driver::execute(SourceStorage const& source) {
  return Evaluator(source.get_analyzed()).evaluate();
}

SourceStorage& Driver::add_source(string const& path) {
  return *this->sources.emplace_back(make_shared<SourceStorage>(path));
}

void Driver::register_main(Node* nd) {
  if (this->entry_point) {
    Error(nd->tok, "duplicate definition of entry point 'main'")
        .add_note(this->entry_point->tok, "already defined here")
        .crash();
  }

  this->entry_point = nd;
}

void Driver::add_error(Error&& e) {
  Driver::get_instance()->errors.emplace_back(std::move(e));
}

Driver* Driver::get_instance() {
  if (!g_instance) {
    g_instance = new Driver();
  }

  return g_instance;
}

CmdOptions Driver::parse_arguments(int argc, char** argv) {
  Vec<string> args;

  CmdOptions opt;

  while (--argc)
    args.emplace_back(*++argv);

  for (auto it = args.begin(); it != args.end();) {
    if (*it == "-h" || *it == "--help") {
      cout << help_string << endl;
      std::exit(0);
    }

    else if (*it == "--version") {
      cout << version_string << endl;
      std::exit(0);
    }

    else {
      opt.run_files.emplace_back(*it++);
    }
  }

  if (opt.run_files.empty()) {
    opt.run_repl = true;
  }

  return opt;
}

} // namespace fire