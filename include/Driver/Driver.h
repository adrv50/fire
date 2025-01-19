#pragma once

#include "typedef.h"
#include "Error.h"

namespace fire {

class SourceStorage;

class Driver {

public:
  Driver* get_instance();

  int main(int argc, char** argv);

  static void add_error(Error&& e);

private:
  Driver();
  ~Driver();

  Vec<SourceStorage> sources;
};

} // namespace fire
