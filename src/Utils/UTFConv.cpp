#include "typedef.h"
#include "Utils.h"

namespace fire::utils::strings {

string to_utf8(u16string const& s) {
  string result;
  for (size_t i = 0; i < s.size(); ++i) {
    uint16_t c = s[i];
    if (c <= 0x7F) {
      result.push_back(static_cast<char>(c));
    }
    else if (c <= 0x7FF) {
      result.push_back(static_cast<char>(0xC0 | (c >> 6)));
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
    else if (c >= 0xD800 && c <= 0xDBFF) { // High surrogate
      if (i + 1 < s.size()) {
        uint16_t low = s[i + 1];
        if (low >= 0xDC00 && low <= 0xDFFF) { // Low surrogate
          uint32_t codepoint = ((c - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
          result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
          result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
          result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          ++i; // Consume low surrogate
        }
        else {
          throw std::runtime_error("Invalid UTF-16 sequence");
        }
      }
      else {
        throw std::runtime_error("Truncated UTF-16 sequence");
      }
    }
    else if (c >= 0xDC00 && c <= 0xDFFF) {
      throw std::runtime_error("Unpaired low surrogate");
    }
    else {
      result.push_back(static_cast<char>(0xE0 | (c >> 12)));
      result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
  }
  return result;
}

u16string to_utf16(string const& s) {
  u16string result;
  for (size_t i = 0; i < s.size(); ++i) {
    uint8_t c = static_cast<uint8_t>(s[i]);
    if (c <= 0x7F) {
      result.push_back(c);
    }
    else if ((c >> 5) == 0x6) { // 2-byte sequence
      if (i + 1 >= s.size())
        throw std::runtime_error("Truncated UTF-8 sequence");
      uint16_t codepoint = ((c & 0x1F) << 6) | (static_cast<uint8_t>(s[i + 1]) & 0x3F);
      result.push_back(codepoint);
      ++i;
    }
    else if ((c >> 4) == 0xE) { // 3-byte sequence
      if (i + 2 >= s.size())
        throw std::runtime_error("Truncated UTF-8 sequence");
      uint16_t codepoint = ((c & 0x0F) << 12) |
                           ((static_cast<uint8_t>(s[i + 1]) & 0x3F) << 6) |
                           (static_cast<uint8_t>(s[i + 2]) & 0x3F);
      result.push_back(codepoint);
      i += 2;
    }
    else if ((c >> 3) == 0x1E) { // 4-byte sequence
      if (i + 3 >= s.size())
        throw std::runtime_error("Truncated UTF-8 sequence");
      uint32_t codepoint = ((c & 0x07) << 18) |
                           ((static_cast<uint8_t>(s[i + 1]) & 0x3F) << 12) |
                           ((static_cast<uint8_t>(s[i + 2]) & 0x3F) << 6) |
                           (static_cast<uint8_t>(s[i + 3]) & 0x3F);
      if (codepoint > 0xFFFF) {
        codepoint -= 0x10000;
        result.push_back(0xD800 + (codepoint >> 10));
        result.push_back(0xDC00 + (codepoint & 0x3FF));
      }
      else {
        result.push_back(static_cast<uint16_t>(codepoint));
      }
      i += 3;
    }
    else {
      throw std::runtime_error("Invalid UTF-8 byte");
    }
  }
  return result;
}

} // namespace fire::utils::strings