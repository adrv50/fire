#pragma once

#include "typedef.h"

struct Token;
class SourceStorage;

//
// SourceLoc: struct for reference of SourceStorage.
//
struct SourceLoc {
private:
  Token* owner;
  SourceStorage const* SS;

public:
  size_t pos;    // position
  size_t length; // location

  size_t line_num = 0;
  size_t pos_in_line = 0;

  string_view get_view() const;
  string_view get_line_view() const;

  tuple<size_t, size_t, size_t> get_line_loc() const;

  SourceStorage const& get_ss() {
    return *this->SS;
  }

  SourceLoc();
  SourceLoc(SourceStorage const* SS, Token* owner, size_t pos, size_t len);
};

//
// SourceStorage: keep data and info of source file.
//
class SourceStorage {

  friend struct SourceLoc;
  friend struct Token;

  mutable Vec<shared_ptr<SourceLoc>> _loc_list;

  Vec<pair<size_t, size_t /* (pos, len) */>> _line_list;

  string path;
  string data;

public:
  shared_ptr<SourceLoc> make_ref(Token* tok, size_t pos, size_t len) const;

  string_view get_view(size_t pos, size_t len) const;

  size_t get_length() const;

  string get_data() const;

  string get_path() const;

  SourceStorage(string const& path);
  ~SourceStorage();
};
