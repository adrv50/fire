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

std::string remove_color(std::string str);
i64 get_length_without_color(std::string const& str);
i64 get_color_length_in_str(std::string const& str);

std::string to_u8string(std::u16string const& str);
std::u16string to_u16string(std::string const& str);

std::string get_base_name(std::string path);

} // namespace utils