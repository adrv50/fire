#pragma once

#include "types.h"

namespace fire {

class FireDriver {
public:
  FireDriver();
  ~FireDriver();

  int fire_main(int argc, char** argv);

  ObjPointer execute(SourceStorage& source);

private:
  Vec<SourceStorage> sources;
};

} // namespace fire