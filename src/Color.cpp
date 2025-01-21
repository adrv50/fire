#include "Color.h"

namespace fire {

Color::Color(u8 r, u8 g, u8 b, u8 a)
    : r(r),
      g(g),
      b(b),
      a(a) {
}

Color::Color(u32 code)
    : code(code) {
}

Color::Color(string const& op)
    : _is_op(true),
      _op(op) {
}

Color::operator string() const {
  static char buf[100];

  if (this->_is_op)
    return "\033[" + this->_op + 'm';

  sprintf(buf, "\e[38;2;%d;%d;%dm", this->r, this->g, this->b);

  return buf;
}

std::ostream& operator<<(std::ostream& ost, Color const& col) {
  return ost << ((string)col);
}

string operator+(string const& s, Color const& c) {
  return s + (string)c;
}

string operator+(Color const& c, string const& s) {
  return (string)c + s;
}

Color Color::Default{"0"};
Color Color::Bold{"1"};
Color Color::Underline{"4"};
Color Color::Unbold{"2"};

Color Color::Black{0, 0, 0};
Color Color::Red{255, 0, 0};
Color Color::Green{0, 255, 0};
Color Color::Yellow{255, 200, 0};
Color Color::Blue{0, 0, 255};
Color Color::Magenta{200, 0, 200};
Color Color::Cyan{60, 140, 255};
Color Color::Gray{60, 60, 60};
Color Color::White{255, 255, 255};

} // namespace fire