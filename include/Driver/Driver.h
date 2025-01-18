#pragma once

#include "typedef.h"

namespace fire {

class SourceStorage;

class Driver {

public:
  Driver();
  ~Driver();

  int main(int argc, char** argv);

private:
  Vec<SourceStorage> sources;
};

} // namespace fire
