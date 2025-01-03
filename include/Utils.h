#pragma once

#include <cstdio>
#include <algorithm>

#include "typedef.h"

//
// utils: utility functions or classes, and tools.
//
namespace utils {

//
// read_text_file:
//   open file from @path, and read to @out
bool read_text_file(string& out, string const& path);

//
// find
template <typename T>
Vec<T>::const_iterator find(Vec<T> const& v, T const& item) {
  return std::find(v.cbegin(), v.cend(), item);
}

//
// join
string join(string const& str, auto const& v, auto to_str_fn) {
  string s;

  for (size_t i = 0; i < v.size(); i++) {
    s += to_str_fn(v[i]);

    if (i + 1 < v.size())
      s += str;
  }

  return s;
}

//
// compare_vector:
//   compare elements and count between two vector.
//
// result:
//   0  = perfectly same.
//  -1  = not same.
template <typename E>
int compare_vector(Vec<E> const& a, Vec<E> const& b, auto cmp_fn) {
  if (a.size() != b.size())
    return -1;

  for (auto it = a.begin(); auto&& e : b)
    if (!cmp_fn(*it, e))
      return -1;

  return 0;
}

//
// format
template <typename... Args>
string format(string const& fmt, Args&&... args) {
  static char buf[0x1000];

  sprintf(buf, fmt.c_str(), std::forward<Args>(args)...);

  return buf;
}

} // namespace utils
