#include <fstream>
#include "SourceStorage.h"

string_view SourceLoc::get_view() const {
  return {this->SS->data.data() + this->pos, this->length};
}

string_view SourceLoc::get_line_view() const {
  auto [_, pos, len] = this->get_line_loc();

  return {this->SS->data.data() + pos, len};
}

tuple<size_t, size_t, size_t> SourceLoc::get_line_loc() const {
  for (size_t ln = 1; auto&& [_pos, _len] : this->SS->_line_list) {
    if (_pos <= this->pos && this->pos + this->length <= _pos + _len) {
      return {ln, _pos, _len};
    }

    ln++;
  }

  throw std::logic_error(string("not found pos") + std::to_string(this->pos) +
                         " in SS->_line_list");
}

SourceLoc::SourceLoc()
    : SourceLoc(nullptr, nullptr, 0, 0) {
}

SourceLoc::SourceLoc(SourceStorage const* SS, Token* owner, size_t pos,
                     size_t len)
    : owner(owner),
      SS(SS),
      pos(pos),
      length(len) {

  auto [ln, begin, end] = this->get_line_loc();

  this->line_num = ln;
  this->pos_in_line = pos - begin + 1;
}

shared_ptr<SourceLoc> SourceStorage::make_ref(Token* tok, size_t pos,
                                              size_t len) const {
  return this->_loc_list.emplace_back(
      make_shared<SourceLoc>(this, tok, pos, len));
}

string_view SourceStorage::get_view(size_t pos, size_t len) const {
  return {this->data.data() + pos, len};
}

size_t SourceStorage::get_length() const {
  return this->data.length();
}

string SourceStorage::get_data() const {
  return this->data;
}

string SourceStorage::get_path() const {
  return this->path;
}

SourceStorage::SourceStorage(string const& path)
    : path(path) {
  std::ifstream ifs{path};

  if (ifs.fail()) {
    throw std::invalid_argument("cannot open path");
  }

  for (string line; std::getline(ifs, line);) {
    line.push_back('\n');

    this->_line_list.emplace_back(this->data.length(), line.length());

    this->data.append(line);
  }
}

SourceStorage::~SourceStorage() {
}
