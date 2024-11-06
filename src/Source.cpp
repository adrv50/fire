
#include "alert.h"
#include "Parser.h"

namespace fire {

Vec<SourceStorage*> ss_v;

SourceStorage& SourceStorage::GetInstance() {
  return **ss_v.rbegin();
}

SourceStorage& SourceStorage::AddIncluded(SourceStorage&& SS) {
  return *this->_included.emplace_back(std::make_shared<SourceStorage>(std::move(SS)));
}

SourceStorage::LineRange SourceStorage::GetLineRange(i64 position) const {
  for (auto&& line : this->line_range_list) {
    if (line.begin <= position && position <= line.end)
      return line;
  }

  throw std::out_of_range("GetLineRange()");
}

std::string_view SourceStorage::GetLineView(LineRange const& line) const {
  return std::string_view(this->data.c_str() + line.begin, line.length);
}

std::vector<SourceStorage::LineRange> SourceStorage::GetLinesOfAST(ASTPointer ast) {
  std::vector<LineRange> lines;

  (void)ast;
  todo_impl;

  return lines;
}

bool SourceStorage::Open() {
  this->file = std::make_shared<std::ifstream>(this->path);

  if (this->file->fail())
    return false;

  i64 pos = 0;

  for (std::string line; std::getline(*this->file, line);) {
    this->line_range_list.emplace_back(this->line_range_list.size(), pos,
                                       pos + line.length());

    this->data += line + '\n';
    pos += line.length() + 1;
  }

  this->data += "\n";

  if (this->line_range_list.empty()) {
    this->line_range_list.emplace_back(0, 0, 0);
  }

  this->file->close();

  return true;
}

bool SourceStorage::IsOpen() const {
  return file && file->is_open();
}

SourceStorage::SourceStorage(std::string path)
    : path(path) {
  ss_v.push_back(this);
}

SourceStorage::~SourceStorage() {
  ss_v.pop_back();
}

} // namespace fire