#include <fstream>
#include <iostream>
#include <unordered_map>

#include "alert.h"
#include "Utils.h"

#include "Lexer.h"
#include "Parser.h"
#include "Sema/Sema.h"

#include "SourceStorage.h"

namespace fire {

static std::unordered_map<string, SourceStorage*> _opened_sources;

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

  throw std::logic_error(string("not found pos=") + std::to_string(this->pos) +
                         ", len=" + std::to_string(this->length) + " in SS->_line_list");
}

SourceLoc::SourceLoc()
    : SourceLoc(nullptr, nullptr, 0, 0) {
}

SourceLoc::SourceLoc(SourceStorage const* SS, Token* owner, size_t pos, size_t len)
    : owner(owner),
      SS(SS),
      pos(pos),
      length(len) {

  (void)this->owner;

  auto [ln, begin, end] = this->get_line_loc();

  this->line_num = ln;
  this->pos_in_line = pos - begin + 1;
}

pair<size_t, size_t>& SourceStorage::append_line(size_t pos, size_t len) {
  return this->_line_list.emplace_back(pos, len);
}

SourceStorage* SourceStorage::import_source(string const& path) const {
  if (!get_opened_instance(path))
    return this->imported.emplace_back(new SourceStorage(path));
  else
    return nullptr;
}

shared_ptr<SourceLoc> SourceStorage::make_ref(Token* tok, size_t pos, size_t len) const {
  return this->_loc_list.emplace_back(make_shared<SourceLoc>(this, tok, pos, len));
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

bool SourceStorage::open(string const& path) {
  if (this->is_open())
    return false; // not closed or duplicate use of instance

  this->fs_path = std::filesystem::absolute(path);
  this->path = this->fs_path.string();

  _opened_sources[this->path] = this;

  this->ifs.reset();
  this->ifs = std::make_unique<std::ifstream>(path);

  if (this->ifs->fail()) {
    std::cout << COL_RED << "fatal error: " << COL_WHITE << "cannot open file '" << path
              << "'" << COL_DEFAULT << std::endl;

    std::exit(1);
  }

  return true;
}

bool SourceStorage::read() {
  if (!this->is_open())
    return false;

  string line;

  while (std::getline(*this->ifs, line)) {
    line.push_back('\n');
    this->append_line(this->data.size(), line.length());
    this->data.append(line);
  }

  return true;
}

bool SourceStorage::is_open() const {
  return (bool)this->ifs && this->ifs->is_open();
}

Token* SourceStorage::get_lexed() const {
  if (!this->lexed) {
    this->_lexer = make_unique<Lexer>(*this);
    this->lexed = this->_lexer->lex();
  }

  return this->lexed;
}

Node* SourceStorage::get_parsed() const {
  if (!this->parsed) {
    this->_parser = make_unique<Parser>(*this, this->get_lexed());
    this->parsed = this->_parser->parse();
  }

  return this->parsed;
}

Node* SourceStorage::get_analyzed() const {
  auto node = this->get_parsed();

  if (!this->_sema) {

    this->_sema = make_unique<sema::Sema>(node);

    this->_sema->check_all();
  }

  return node;
}

SourceStorage* SourceStorage::get_opened_instance(string const& path) {
  return _opened_sources[std::filesystem::absolute(path).string()];
}

SourceStorage::SourceStorage() {
  this->append_line(0, 0);
}

SourceStorage::SourceStorage(string const& path)
    : SourceStorage() {
  this->open(path);
  this->read();
}

SourceStorage::~SourceStorage() {
}

} // namespace fire