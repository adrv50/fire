#pragma once

#include "types.h"

namespace fire {

struct Color {
  u8 r, g, b;

  Color(u8 r, u8 g, u8 b)
      : r(r),
        g(g),
        b(b) {
  }

  // if repeat
  Color(u8 x)
      : Color(x, x, x) {
  }

  operator std::string() const {
    char buf[50]{};

    sprintf(buf, "\e[38;2;%d;%d;%dm", r, g, b);

    return buf;
  }

  static const Color White;
  static const Color Gray;
  static const Color DarkGray;
  static const Color Black;
  static const Color Red;
  static const Color Yellow;
  static const Color Blue;
  static const Color Magenta;
  static const Color Cyan;
};

static inline std::ostream& operator<<(std::ostream& ost,
                                       Color const& c) {
  return ost << (std::string)c;
}

} // namespace fire