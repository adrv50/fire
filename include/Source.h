#pragma once

#include <any>
#include <algorithm>
#include <concepts>
#include <string>
#include <fstream>

#include "types.h"

namespace fire {

struct Token;

struct SourceStorage {
  struct LineRange {
    i64 index, begin, end, length;

    LineRange(i64 index, i64 begin, i64 end)
        : index(index),
          begin(begin),
          end(end),
          length(end - begin) {
    }

    LineRange()
        : LineRange(0, 0, 0) {
    }
  };

  std::string path;
  std::string data;

  std::shared_ptr<std::ifstream> file;

  std::vector<LineRange> line_range_list;

  std::vector<Token> token_list;

  static SourceStorage& GetInstance();

  SourceStorage& AddIncluded(SourceStorage&& SS);

  bool Open();
  bool IsOpen() const;

  LineRange GetLineRange(i64 position) const;
  std::string_view GetLineView(LineRange const& line) const;
  std::vector<LineRange> GetLinesOfAST(ASTPointer ast);

  std::string_view GetLineView(i64 index) const {
    return this->GetLineView(this->line_range_list[index]);
  }

  int Count() const {
    return this->line_range_list.size();
  }

  char operator[](size_t N) const {
    return this->data[N];
  }

  SourceStorage(std::string path);
  ~SourceStorage();

private:
  std::vector<std::shared_ptr<SourceStorage>> _included;
};

struct SourceLocation {
  i64 position;
  i64 length;
  i64 pos_in_line;

  SourceStorage* ref;
  SourceStorage::LineRange line;

  std::string_view GetLine() const {
    return this->ref->GetLineView(this->line);
  }

  SourceLocation(i64 pos, i64 len, SourceStorage* r)
      : position(pos),
        length(len),
        pos_in_line(0),
        ref(r) {
    if (this->ref) {
      this->line = this->ref->GetLineRange(this->position);
      this->pos_in_line = this->position - this->line.begin;
    }
  }

  SourceLocation()
      : SourceLocation(0, 0, nullptr) {
  }
};

} // namespace fire