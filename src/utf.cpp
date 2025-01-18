#include <codecvt>
#include <locale>

#include "utf.h"

namespace fire::utf {

static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;

string to_utf8(std::u16string const& str) {
  return conv.to_bytes(str);
}

std::u16string to_utf16(string const& str) {
  return conv.from_bytes(str);
}

} // namespace fire::utf