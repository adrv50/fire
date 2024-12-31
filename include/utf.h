#pragma once

#include <string>
#include "typedef.h"

namespace utf {

string to_utf8(std::u16string const& str);

std::u16string to_utf16(string const& str);

} // namespace utf