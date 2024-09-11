#pragma once

#include <any>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>
#include "types.h"

namespace utils {

template <class... Args>
std::string Format(std::string const& fmt, Args&&... args) {
  static char buf[0x1000];
  sprintf(buf, fmt.c_str(), std::forward<Args>(args)...);
  return buf;
}

std::string to_u8string(std::u16string const& str);
std::u16string to_u16string(std::string const& str);

} // namespace utils