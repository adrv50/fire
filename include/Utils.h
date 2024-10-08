#pragma once

#include <any>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>
#include <functional>

#include "types.h"

namespace utils {

template <class... Args>
string Format(string const& fmt, Args&&... args) {
  static char buf[0x1000];
  sprintf(buf, fmt.c_str(), std::forward<Args>(args)...);
  return buf;
}

template <class T>
string join(string const& s, vector<T> const& vec, auto tostrfn) {
  string ret;

  for (i64 i = 0; i < (i64)vec.size(); i++) {
    ret += tostrfn(vec[i]);

    if (i + 1 < (i64)vec.size())
      ret += s;
  }

  return ret;
}

template <typename E>
bool contains(Vec<E> const& v, E const& e) {
  return std::find(v.begin(), v.end(), e) != v.end();
}

string remove_color(string str);
i64 get_length_without_color(string const& str);
i64 get_color_length_in_str(string const& str);

string to_u8string(std::u16string const& str);
std::u16string to_u16string(string const& str);

string get_base_name(string path);

} // namespace utils