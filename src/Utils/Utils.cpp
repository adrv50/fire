#include "Utils/Utils.h"

namespace utils {

static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,
                            char16_t>
    conv;

std::string to_u8string(std::u16string const& str) {
  return conv.to_bytes(str);
}

std::u16string to_u16string(std::string const& str) {
  return conv.from_bytes(str);
}

} // namespace utils