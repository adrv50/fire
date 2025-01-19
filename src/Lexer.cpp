#include "utf.h"
#include "Token/Token.h"
#include "Lexer.h"
#include "Error.h"

namespace fire {

using TKop = TokenOperatorKind;
using TKpunct = TokenPunctKind;
using TKkwd = TokenKwdKind;

//
// all punctuators or operators
//
static constexpr char const* all_punct_list[] = {
    "...", "<<=", ">>=", "[[", "]]", "<<", ">>", "=>", "<=", ">=", "==", "!=", "..",
    "+=",  "-=",  "*=",  "/=", "%=", "&=", "^=", "|=", "&&", "||", "->", "::", "<",
    ">",   "+",   "-",   "/",  "*",  "%",  "=",  ";",  ":",  ",",  ".",  "[",  "]",
    "(",   ")",   "{",   "}",  "!",  "?",  "&",  "^",  "|",  "@",
};

//
// string representation for TokenKind
//
static char const* s_kind[] = {
    "(unknown)", "decimal",   "float",   "hexadecimal", "binary",
    "string",    "character", "boolean", "identifier",  "punctuator",
    "\";\"",     "operator",  "end",
};

//
// tok_operators:
//   pairs of TokenOperatorKind and its string representation
//
static constexpr pair<TKop, char const*> tok_operators[] = {
    {TKop::None, ""},

    {TKop::MemberAccess, "."},
    {TKop::SubscriptionOpen, "["},
    {TKop::SubscriptionClose, "]"},

    {TKop::Add, "+"},
    {TKop::Sub, "-"},
    {TKop::Mul, "*"},
    {TKop::Div, "/"},
    {TKop::Mod, "%"},
    {TKop::Assign, "="},

    {TKop::LShift, "<<"},
    {TKop::RShift, ">>"},

    {TKop::LeftBig, ">"},
    {TKop::RightBig, "<"},
    {TKop::LeftBigOrEq, ">="},
    {TKop::RightBigOrEq, "<="},
    {TKop::Equal, "=="},
    {TKop::NotEqual, "!="},

    {TKop::BitAnd, "&"},
    {TKop::BitOr, "|"},
    {TKop::BitXor, "^"},

    {TKop::BitAndAssign, "&="},
    {TKop::BitOrAssign, "|="},
    {TKop::BitXorAssign, "^="},

    {TKop::LShiftAssign, "<<="},
    {TKop::RShiftAssign, ">>="},

    {TKop::AddAssign, "+="},
    {TKop::SubAssign, "-="},
    {TKop::MulAssign, "*="},
    {TKop::DivAssign, "/="},
    {TKop::ModAssign, "%="},
};

//
//  tok_punctuators:
//    pairs of TokenPunctKind and its string representation
//
static constexpr pair<TKpunct, char const*> tok_punctuators[] = {
    {TKpunct::None, ""},

    {TKpunct::Comma, ","},
    {TKpunct::Dot, "."},

    {TKpunct::Semi, ";"},
    {TKpunct::Colon, ":"},

    {TKpunct::ScopeResol, "::"},

    {TKpunct::ResultTypeSpecifier, "->"},

    {TKpunct::CaseMatch, "=>"},

    {TKpunct::BraceOpen, "("},
    {TKpunct::BraceClose, ")"},
    {TKpunct::BlockBraceOpen, "{"},
    {TKpunct::BlockBraceClose, "}"},
    {TKpunct::AngleBraceOpen, "<"},
    {TKpunct::AngleBraceClose, ">"},
    {TKpunct::ArrayBraceOpen, "["},
    {TKpunct::ArrayBraceClose, "]"},

    {TKpunct::AttributeBegin, "[["},
    {TKpunct::AttributeEnd, "]]"},

    {TKpunct::Ellipsis, "..."},
};

//
// tok_keywords:
//   pairs of TokenKwdKind and its string representation
//
static constexpr pair<TKkwd, char const*> tok_keywords[] = {
    {TKkwd::None, ""},

    {TKkwd::Namespace, "namespace"},

    // function
    {TKkwd::Func, "fn"},

    // type definition
    {TKkwd::Enum, "enum"},
    {TKkwd::Class, "class"},
    {TKkwd::Struct, "struct"},
    {TKkwd::Namespace, "namespace"},

    // concept definition
    {TKkwd::Concept, "concept"},

    // let statement (variable declaration)
    {TKkwd::Let, "let"},

    // qualifiers for let-stmt
    {TKkwd::Mut, "mut"},
    {TKkwd::Ref, "ref"},

    // in
    {TKkwd::In, "in"},

    // control flow
    {TKkwd::If, "if"},
    {TKkwd::Else, "else"},
    {TKkwd::Switch, "switch"},
    {TKkwd::Case, "case"},
    {TKkwd::Default, "default"},
    {TKkwd::Match, "match"},
    {TKkwd::For, "for"},
    {TKkwd::Loop, "loop"},
    {TKkwd::Do, "do"},
    {TKkwd::While, "while"},

    // return statement
    {TKkwd::Return, "return"},

    // break statement
    {TKkwd::Break, "break"},

    // continue statement
    {TKkwd::Continue, "continue"},

    // logical operators
    {TKkwd::Not, "not"},
    {TKkwd::And, "and"},
    {TKkwd::Or, "or"},
    {TKkwd::Cast, "cast"},

    // boolean literals
    {TKkwd::True, "true"},
    {TKkwd::False, "false"},

    // built-in type names
    {TKkwd::Int, "int"},
    {TKkwd::Float, "float"},
    {TKkwd::Bool, "bool"},
    {TKkwd::Char, "char"},
    {TKkwd::String, "string"},
    {TKkwd::Vector, "vector"},
    {TKkwd::Tuple, "tuple"},
    {TKkwd::Dict, "dict"},
    {TKkwd::Functor, "func"},
};

string Token::kind_to_str(TokenKind k) {
  return s_kind[static_cast<size_t>(k)];
}

string Token::kwd_to_str(TokenKwdKind k) {
  return tok_keywords[static_cast<size_t>(k)].second;
}

string Token::op_to_str(TokenOperatorKind k) {
  return tok_operators[static_cast<size_t>(k)].second;
}

string Token::punct_to_str(TokenPunctKind k) {
  return tok_punctuators[static_cast<size_t>(k)].second;
}

bool Lexer::check(int add) const {
  return this->pos + add <= this->len;
}

char Lexer::peek(int offset) const {
  return this->SS.get_data()[this->pos + offset];
}

string_view Lexer::get(int len) const {
  return this->check(len) ? this->SS.get_view(this->pos, len) : "";
}

bool Lexer::eat(string_view s, bool keep_pos) {
  if (this->check() && this->get(s.length()) == s) {
    if (!keep_pos)
      this->pos += s.length();

    return true;
  }

  return false;
}

bool Lexer::match(string_view s) {
  return this->eat(s, true);
}

string Lexer::trim_hexadecimal() {
  string s;

  for (char c; this->check() && isxdigit((c = this->peek())); this->pos++)
    s += c;

  return s;
}

string Lexer::trim_binary() {
  string s;

  for (char c; this->check() && ((c = this->peek()) == '0' || c == '1'); this->pos++)
    s += c;

  return s;
}

string Lexer::trim_decimal() {
  string s;

  for (char c; this->check() && isdigit((c = this->peek())); this->pos++)
    s += c;

  return s;
}

string Lexer::trim_identifier() {
  string s;

  for (char c; this->check() && (isalnum((c = this->peek())) || c == '_'); this->pos++)
    s += c;

  return s;
}

//
// pass_space:
//
void Lexer::pass_space() {
  while (this->check() && isspace(this->peek()))
    this->pos++;
}

//
// ctor
//
Lexer::Lexer(SourceStorage& SS)
    : SS(SS),
      pos(0),
      len(SS.get_length()) {
}

//
// do lex
//
Token* Lexer::lex() {
  auto top = Token::make();
  auto cur = top;

  this->pos = 0;

  this->pass_space();

  while (this->check()) {

    char c = this->peek();
    size_t _pos = this->pos;

    //
    // pass comment line
    if (this->eat("//")) {
      while (this->check() && this->peek() != '\n')
        this->pos++;

      this->pass_space();
      continue;
    }

    //
    // pass comment block
    if (this->eat("/*")) {
      while (this->check() && !this->eat("*/"))
        this->pos++;

      this->pass_space();
      continue;
    }

    //
    // hexadecimal
    if (this->eat("0x") || this->eat("0X")) {
      cur = Token::make(TokenKind::Hexadecimal, &this->SS, cur, this->trim_hexadecimal(),
                        _pos);

      cur->literal_data.v_hex = std::stoull(cur->str, nullptr, 16);
    }

    //
    // binary
    else if (this->eat("0b") || this->eat("0B")) {
      cur = Token::make(TokenKind::Binary, &this->SS, cur, this->trim_binary(), _pos);

      cur->literal_data.v_bin = std::stoull(cur->str, nullptr, 2);
    }

    //
    // decimal or float
    else if (isdigit(c)) {
      cur = Token::make(TokenKind::Decimal, &this->SS, cur, this->trim_decimal(), _pos);

      // if eat dot, it is a float
      if (this->match(".") && isdigit(this->peek(1))) {
        this->pos++;

        cur->kind = TokenKind::Float;
        cur->str += "." + this->trim_decimal();
        cur->literal_data.v_float = std::stod(cur->str);
      }

      // else => int
      else {
        cur->literal_data.v_int = std::stoll(cur->str);
      }
    }

    //
    // char
    else if (this->eat("'")) {
      string s;

      for (; this->check() && (c = this->peek()) != '\''; this->pos++)
        s += c;

      this->pos++;

      cur = Token::make(TokenKind::Character, &this->SS, cur, s, _pos);

      auto s16 = utf::to_utf16(s);

      if (s16.length() != 1)
        Error(cur, "invalid character literal").crash();

      cur->literal_data.v_char = s16[0];
    }

    //
    // string
    else if (this->eat("\"")) {
      string s;

      for (; this->check() && (c = this->peek()) != '"'; this->pos++)
        s += c;

      this->pos++;

      cur = Token::make(TokenKind::String, &this->SS, cur, s, _pos);

      cur->v_str = utf::to_utf16(s);
    }

    //
    // identifier
    else if (isalpha(c) || c == '_') {
      cur = Token::make(TokenKind::Identifier, &this->SS, cur, this->trim_identifier(),
                        _pos);

      for (auto&& [k, s] : tok_keywords)
        if (cur->str == s) {
          cur->set_kwd(k);
          break;
        }
    }

    //
    // find punctuator
    else {
      for (auto s : all_punct_list)
        if (this->eat(s)) {
          cur = Token::make(TokenKind::Punctuator, &this->SS, cur, s, _pos);
          goto _found;
        }

      //
      // set location for show error
      if (auto src_loc = cur->ref) {
        src_loc->pos = this->pos;
        src_loc->length = 1;
      }

      // Error(cur, "invalid token: '" + string(1, c) + "'").emit().stop();

      throw std::logic_error("invalid token: '" + string(1, c) + "'");

    _found:;

      //
      // check operators
      for (auto&& [k, s] : tok_operators)
        if (cur->str == s) {
          cur->set_op(k);
          break;
        }

      //
      // check punctuators
      for (auto&& [k, s] : tok_punctuators)
        if (cur->str == s) {
          if (k == TokenPunctKind::Semi)
            cur->kind = TokenKind::Semi;
          else
            cur->set_punct(k);

          break;
        }
    }

    this->pass_space();
  }

  cur = Token::make(TokenKind::End, &this->SS, cur, "", this->pos);

  auto ret = top->next;
  top->next = nullptr;

  return ret;
}

} // namespace fire