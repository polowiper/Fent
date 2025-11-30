#pragma once
#include <string>
#include <variant>

enum class TokenKind {
  LBrace,
  RBrace,
  Identifier,
  Number,
  Operator,
  Equals,
  Not,
  EndOfFile,
  Unknown
};

using LiteralValue = std::variant<std::monostate, int, char, bool>;

struct Token {
  TokenKind kind;
  std::string lexeme;
  LiteralValue literal;
};
