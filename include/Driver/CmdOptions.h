#pragma once

#include "../typedef.h"

namespace fire {

struct CmdOptions {

  bool run_repl = false;

  Vec<string> run_files;
};

} // namespace fire