//
// keep temporary string for std::string_view
//

#pragma once

#include <string>
#include <vector>

namespace fire {

class strpool {

  std::vector<std::string*> pool;

  static inline strpool* _inst = nullptr;

  static strpool* get_instance() {
    if (!_inst)
      _inst = new strpool();

    return _inst;
  }

  strpool() = default;

public:
  static std::string* find(std::string_view sv) {
    for (auto&& s : get_instance()->pool)
      if (*s == sv)
        return s;

    return nullptr;
  }

  static std::string_view get(std::string const& s) {
    auto p = find(s);

    if (!p)
      p = &add(s);

    return std::string_view(*p);
  }

  static std::string& add(std::string const& s) {
    return *get_instance()->pool.emplace_back(new std::string(s));
  }
};

} // namespace fire