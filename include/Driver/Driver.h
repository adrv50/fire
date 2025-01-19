#pragma once

#include "typedef.h"
#include "Driver/Error.h"

#include "CmdOptions.h"

namespace fire {

class SourceStorage;

class Driver {

  CmdOptions opt;

  Node* entry_point = nullptr;

public:
  int main(int argc, char** argv);

  Obj execute(SourceStorage const& source);

  SourceStorage& add_source(string const& path);

  void register_main(Node* main_fn_nd);

  static void add_error(Error&& e);

  static Driver* get_instance();

private:
  Driver();
  ~Driver();

  static CmdOptions parse_arguments(int argc, char** argv);

  Vec<shared_ptr<SourceStorage>> sources;

  Vec<Error> errors;
};

} // namespace fire
