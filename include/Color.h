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

  Color(u8 r, u8 g, u8 b, u8 a = 255);
  Color(u32 code);

  operator string() const;
};

std::ostream& operator<<(std::ostream&, Color const&);

} // namespace fire