#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include "types.h"

namespace metro {

struct SourceStorage {
  struct LineRange {
    i64 index;
    i64 begin;
    i64 end;
    i64 length;

    LineRange(i64 index, i64 begin, i64 end)
        : index(index), begin(begin), end(end), length(end - begin) {
    }

    LineRange() : LineRange(0, 0, 0) {
    }
  };

  std::string path;
  std::string data;
  std::unique_ptr<std::ifstream> file;
  std::vector<LineRange> line_range_list;

  LineRange GetLineRange(i64 position) const;
  std::string_view GetLineView(LineRange const& line) const;

  std::vector<LineRange> GetLinesOfAST(AST::ASTPointer ast);

  char operator[](size_t index) const {
    return this->data[index];
  }

  bool Open();
  bool IsOpen() const;

  explicit SourceStorage(std::string path);
};

// reference to any location in SourceStorage
struct SourceLocation {
public:
  i64 position;
  i64 length;

  SourceStorage const* ref;
  SourceStorage::LineRange line;

  std::string_view GetLine() const {
    return ref->GetLineView(line);
  }

  SourceLocation(i64 pos, i64 len, SourceStorage const* S)
      : position(pos), length(len), ref(S) {
    if (S)
      this->line = S->GetLineRange(pos);
  }

  SourceLocation() : SourceLocation(0, 0, nullptr) {
  }
};

} // namespace metro