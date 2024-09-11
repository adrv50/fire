
#include "alert.h"
#include "Parser/Source.h"

namespace metro {

SourceStorage::LineRange
SourceStorage::GetLineRange(i64 position) const {
  for (auto&& line : this->line_range_list) {
    if (line.begin <= position && position <= line.end)
      return line;
  }

  throw std::out_of_range("GetLineRange()");
}

std::string_view
SourceStorage::GetLineView(LineRange const& line) const {
  return std::string_view(this->data.c_str() + line.begin,
                          line.length);
}

std::vector<SourceStorage::LineRange>
SourceStorage::GetLinesOfAST(AST::ASTPointer ast) {
}

bool SourceStorage::Open() {
  this->file = std::make_unique<std::ifstream>(this->path);

  if (this->file->fail())
    return false;

  i64 pos = 0;

  for (std::string line; std::getline(*this->file, line);) {
    this->line_range_list.emplace_back(this->line_range_list.size(),
                                       pos, pos + line.length());

    this->data += line + '\n';
    pos += line.length() + 1;
  }

  return true;
}

bool SourceStorage::IsOpen() const {
  return file && !file->fail();
}

SourceStorage::SourceStorage(std::string path) : path(path) {
}

} // namespace metro