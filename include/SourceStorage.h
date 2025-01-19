#pragma once

#include <iosfwd>
#include "typedef.h"

namespace fire {

struct Token;
struct Node;
class SourceStorage;

class Lexer;
class Parser;
class Sema;
class Driver;

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

  bool is_in_repl() const;

  string_view get_view() const;
  string_view get_line_view() const;

  tuple<size_t, size_t, size_t> get_line_loc() const;

  SourceStorage const* get_ss() {
    return this->SS;
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

  friend class Parser;
  friend class Sema;
  friend class Driver;

  mutable Vec<shared_ptr<SourceLoc>> _loc_list;

  unique_ptr<std::ifstream> ifs;

  Vec<pair<size_t, size_t /* (pos, len) */>> _line_list;

  string path;
  string data;

  Vec<shared_ptr<SourceStorage>> imported;

  bool is_in_repl = false;

  mutable Token* lexed = nullptr;
  mutable Node* parsed = nullptr;

  mutable unique_ptr<Lexer> _lexer;
  mutable unique_ptr<Parser> _parser;

  pair<size_t, size_t>& append_line(size_t pos, size_t len);

  shared_ptr<SourceStorage> import_source(string const& path);

public:
  shared_ptr<SourceLoc> make_ref(Token* tok, size_t pos, size_t len) const;

  string_view get_view(size_t pos, size_t len) const;

  string_view get_line_view(size_t index) const;

  size_t get_length() const;

  string get_data() const;

  string get_path() const;

  bool open(string const& path);

  // void close();

  bool read();

  bool is_open() const;

  Token* get_lexed() const;

  Node* get_parsed() const;

  SourceStorage();

  SourceStorage(string const& path);

  ~SourceStorage();
};

} // namespace fire