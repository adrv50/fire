#pragma once

#include <iostream>

#include "typedef.h"

namespace fire {

struct Color {
  union {
    struct {
      u8 r, g, b, a;
    };

    u32 code;
  };

  bool _is_op = false;
  string _op;

  Color(u8 r, u8 g, u8 b, u8 a = 255);
  Color(u32 code);
  explicit Color(string const& op);

  operator string() const;

  static Color Default;
  static Color Bold;
  static Color Underline;
  static Color Unbold;

  static Color Black;
  static Color Red;
  static Color DarkRed;
  static Color Pink;
  static Color LightGreen;
  static Color Green;
  static Color Yellow;
  static Color Orange;
  static Color Blue;
  static Color DarkBlue;
  static Color Magenta;
  static Color Purple;
  static Color Cyan;
  static Color Gray;
  static Color White;
};

std::ostream& operator<<(std::ostream&, Color const&);

string operator+(string const&, Color const&);
string operator+(Color const&, string const&);

} // namespace fire