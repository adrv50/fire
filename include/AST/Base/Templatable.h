#pragma once

namespace fire::AST {

struct Templatable : Named {

  struct ParameterName {
    Token token;
    Vec<ParameterName> params;

    string to_string() const {
      string s = token.str;

      if (params.size() >= 1) {
        s += "<" +
             utils::join(", ", params,
                         [](auto const& P) -> string {
                           return P.to_string();
                         }) +
             ">";
      }

      return s;
    }
  };

  Token TemplateTok; // "<"

  bool IsTemplated = false;

  bool IsInstantiated = false;

  Vec<ParameterName> ParameterList;

  //
  // 可変長パラメータ引数
  bool IsVariableParameters = false;
  Token VariableParams_Name;

  size_t ParameterCount() const {
    return ParameterList.size();
  }

  // ASTPtr<Block> owner_block_ptr = nullptr;
  // size_t index_of_self_in_owner_block_list = 0;

  // ASTPointer& InsertInstantiated(ASTPtr<Templatable> ast);

protected:
  Templatable(ASTKind kind, Token tok, Token name)
      : Named(kind, tok, name) {
    this->_attr.IsTemplate = true;
  }

  Templatable(ASTKind k, Token t)
      : Templatable(k, t, t) {
  }

  void _Copy(Templatable const& T) {

    this->ScopeCtxPtr = T.ScopeCtxPtr;

    this->TemplateTok = T.TemplateTok;

    this->IsTemplated = T.IsTemplated;

    this->IsInstantiated = T.IsInstantiated;

    for (auto&& Param : T.ParameterList) {
      this->ParameterList.emplace_back(Param);
    }
  }
};

} // namespace fire::AST