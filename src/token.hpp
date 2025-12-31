#pragma once
#include <string>
#include <variant>
#define DEBUG 1
enum class TokenKind {
  // Delimiters
  LBrace,
  RBrace,
  Lpar,
  Rpar,
  Semicolon,
  Comma,

  // Literals and identifiers
  Identifier,
  Number,
  String,
  True,
  False,

  // Keywords
  If,
  Else,
  While,
  Return,
  Const,
  Define,

  // Operators
  Plus,
  Minus,
  Multiply,
  Divide,
  Modulo,
  Equals,
  EqualEqual,
  Less,
  Greater,
  Not,
  And,
  Or,

  // Control
  EndOfFile,
  Unknown
};

using LiteralValue = std::variant<std::monostate, int, bool, std::string>;

struct Position {
  int line = 1;
};

struct Token {
  Position pos;
  TokenKind kind;
  std::string lexeme;
  LiteralValue literal;
};
