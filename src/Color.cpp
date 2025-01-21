#include "Color.h"

namespace fire {

Color::Color(u8 r, u8 g, u8 b, u8 a = 255)
    : r(r),
      g(g),
      b(b),
      a(a) {
}

Color::Color(u32 code)
    : code(code) {
}

Color::operator string() const {
  static char buf[10];

  sprintf(buf, "\e[38;2;%d;%d;%dm", this->r, this->g, this->b);

  return buf;
}

std::ostream& operator<<(std::ostream& ost, Color const& col) {
  return ost << ((string)col);
}

} // namespace fire