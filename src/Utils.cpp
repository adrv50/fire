#include <fstream>
#include "Utils.h"

namespace fire::utils {

//
// read_text_file:
//   open file from @path, and read to @out
//
bool read_text_file(string& out, string const& path) {
  std::ifstream ifs{path};

  if (ifs.fail())
    return false;

  for (string line; std::getline(ifs, line);) {
    line.push_back('\n');
    out.append(line);
  }

  return true;
}

} // namespace fire::utils