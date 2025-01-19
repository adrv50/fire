#pragma once

#include "typedef.h"
#include "Error.h"

#include "CmdOptions.h"

namespace fire {

class SourceStorage;

class Driver {

public:
  int main(int argc, char** argv);

  Obj execute(SourceStorage const& source);

  static void add_error(Error&& e);

  static Driver* get_instance();

private:
  Driver();
  ~Driver();

  static CmdOptions parse_arguments(int argc, char** argv);

  Vec<SourceStorage> sources;

  Vec<Error> errors;
};

} // namespace fire
