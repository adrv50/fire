#include "Utils.h"

namespace utils {

static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,
                            char16_t>
    conv;

std::string remove_color(std::string str) {
  size_t pos = 0;

  while ((pos = str.find('\e')) != std::string::npos) {
    size_t i = pos + 1;

    while (i < str.length() && str[i++] != 'm')
      ;

    str.erase(str.begin() + pos, str.begin() + i);
  }

  return str;
}

i64 get_length_without_color(std::string const& str) {
  return remove_color(str).length();
}

i64 get_color_length_in_str(std::string const& str) {
  return str.length() - get_length_without_color(str);
}

std::string to_u8string(std::u16string const& str) {
  return conv.to_bytes(str);
}

std::u16string to_u16string(std::string const& str) {
  return conv.from_bytes(str);
}

std::string get_base_name(std::string path) {
  if (auto slash = path.rfind('/'); slash != std::string::npos)
    path = path.substr(slash + 1);

  return path.substr(0, path.rfind('.'));
}

} // namespace utils