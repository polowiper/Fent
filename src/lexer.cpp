#include "lexer.hpp"
#include "token.hpp"
#include <cctype>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace std;

static bool isAtEnd(size_t pos, string_view source) {
  return pos >= source.length();
}

static char peek(size_t pos, string_view source) {
  if (isAtEnd(pos, source))
    return '\0';
  return source[pos];
}

// Peek ahead by offset
static char peekNext(size_t pos, string_view source, int offset = 1) {
  if (pos + offset >= source.length())
    return '\0';
  return source[pos + offset];
}

static TokenKind matchKeyword(const string &text) {
  static const map<string, TokenKind> keywords = {
      {"if", TokenKind::If},       {"else", TokenKind::Else},
      {"while", TokenKind::While}, {"return", TokenKind::Return},
      {"const", TokenKind::Const}, {"true", TokenKind::True},
      {"false", TokenKind::False}, {"define", TokenKind::Define},
  };

  auto it = keywords.find(text);
  if (it != keywords.end())
    return it->second;
  return TokenKind::Identifier;
}

static Token scanNumber(string_view source, size_t &pos, int line) {
  Token tok;
  tok.pos.line = line;
  tok.kind = TokenKind::Number;

  size_t start = pos;
  while (!isAtEnd(pos, source) && isdigit(peek(pos, source))) {
    pos++;
  }

  // Handle decimal point
  if (peek(pos, source) == '.' && isdigit(peekNext(pos, source))) {
    pos++; // consume '.'
    while (!isAtEnd(pos, source) && isdigit(peek(pos, source))) {
      pos++;
    }
  }

  tok.lexeme = string(source.substr(start, pos - start));
  tok.literal =
      stoi(tok.lexeme); // For now just use int, could be float later idfk
  // Edit : Uhm well yeah fuck floating point values
  return tok;
}

// Scan a string literal
static Token scanString(string_view source, size_t &pos, int &line) {
  Token tok;
  tok.pos.line = line;
  tok.kind = TokenKind::String;

  size_t start = pos;
  pos++; // consume opening quote

  while (!isAtEnd(pos, source) && peek(pos, source) != '"') {
    if (peek(pos, source) == '\n')
      line++;
    pos++;
  }

  if (isAtEnd(pos, source)) {
    tok.kind = TokenKind::Unknown;
    tok.lexeme = "unterminated string";
    return tok;
  }

  pos++; // consume closing quote
  tok.lexeme =
      string(source.substr(start + 1, pos - start - 2)); // exclude quotes
  tok.literal = tok.lexeme;
  return tok;
}

static Token scanIdentifier(string_view source, size_t &pos, int line) {
  Token tok;
  tok.pos.line = line;

  size_t start = pos;
  while (!isAtEnd(pos, source) && isalnum(peek(pos, source))) {
    pos++;
  }

  tok.lexeme = string(source.substr(start, pos - start));
  tok.kind = matchKeyword(tok.lexeme);

  if (tok.kind == TokenKind::True)
    tok.literal = true;
  else if (tok.kind == TokenKind::False)
    tok.literal = false;

  return tok;
}

vector<Token> lex(string_view source) {
  vector<Token> tokens;
  size_t pos = 0;
  int line = 1;

  while (!isAtEnd(pos, source)) {
    char c = peek(pos, source);

    // Skip whitespace
    if (isspace(c)) {
      if (c == '\n')
        line++;
      pos++;
      continue;
    }

    Token tok;
    tok.pos.line = line;

    // Single-character tokens
    switch (c) {
    case '{':
      tok.kind = TokenKind::LBrace;
      tok.lexeme = "{";
      pos++;
      tokens.push_back(tok);
      continue;

    case '}':
      tok.kind = TokenKind::RBrace;
      tok.lexeme = "}";
      pos++;
      tokens.push_back(tok);
      continue;

    case '(':
      tok.kind = TokenKind::Lpar;
      tok.lexeme = "(";
      pos++;
      tokens.push_back(tok);
      continue;

    case ')':
      tok.kind = TokenKind::Rpar;
      tok.lexeme = ")";
      pos++;
      tokens.push_back(tok);
      continue;

    case ';':
      tok.kind = TokenKind::Semicolon;
      tok.lexeme = ";";
      pos++;
      tokens.push_back(tok);
      continue;

    case ',':
      tok.kind = TokenKind::Comma;
      tok.lexeme = ",";
      pos++;
      tokens.push_back(tok);
      continue;

    case '+':
      tok.kind = TokenKind::Plus;
      tok.lexeme = "+";
      pos++;
      tokens.push_back(tok);
      continue;

    case '-':
      tok.kind = TokenKind::Minus;
      tok.lexeme = "-";
      pos++;
      tokens.push_back(tok);
      continue;

    case '*':
      tok.kind = TokenKind::Multiply;
      tok.lexeme = "*";
      pos++;
      tokens.push_back(tok);
      continue;

    case '/':
      tok.kind = TokenKind::Divide;
      tok.lexeme = "/";
      pos++;
      tokens.push_back(tok);
      continue;

    case '%':
      tok.kind = TokenKind::Modulo;
      tok.lexeme = "%";
      pos++;
      tokens.push_back(tok);
      continue;

    case '<':
      tok.kind = TokenKind::Less;
      tok.lexeme = "<";
      pos++;
      tokens.push_back(tok);
      continue;

    case '>':
      tok.kind = TokenKind::Greater;
      tok.lexeme = ">";
      pos++;
      tokens.push_back(tok);
      continue;

    case '!':
      tok.kind = TokenKind::Not;
      tok.lexeme = "!";
      pos++;
      tokens.push_back(tok);
      continue;

    case '=':
      // Check for ==
      if (peekNext(pos, source) == '=') {
        tok.kind = TokenKind::EqualEqual;
        tok.lexeme = "==";
        pos += 2;
      } else {
        tok.kind = TokenKind::Equals;
        tok.lexeme = "=";
        pos++;
      }
      tokens.push_back(tok);
      continue;

    case '"':
      tokens.push_back(scanString(source, pos, line));
      continue;
    }

    // Numbers
    if (isdigit(c)) {
      tokens.push_back(scanNumber(source, pos, line));
      continue;
    }

    // Identifiers and keywords
    if (isalpha(c)) {
      tokens.push_back(scanIdentifier(source, pos, line));
      continue;
    }

    // Unknown character
    tok.kind = TokenKind::Unknown;
    tok.lexeme = string(1, c);
    tokens.push_back(tok);
    pos++;
  }

  // Add EOF token
  Token eof;
  eof.pos.line = line;
  eof.kind = TokenKind::EndOfFile;
  eof.lexeme = "";
  tokens.push_back(eof);

  return tokens;
}

string tokenKindToString(TokenKind kind) {
  switch (kind) {
  case TokenKind::LBrace:
    return "LBrace";
  case TokenKind::RBrace:
    return "RBrace";
  case TokenKind::Lpar:
    return "Lpar";
  case TokenKind::Rpar:
    return "Rpar";
  case TokenKind::Semicolon:
    return "Semicolon";
  case TokenKind::Comma:
    return "Comma";
  case TokenKind::Identifier:
    return "Identifier";
  case TokenKind::Number:
    return "Number";
  case TokenKind::String:
    return "String";
  case TokenKind::True:
    return "True";
  case TokenKind::False:
    return "False";
  case TokenKind::If:
    return "If";
  case TokenKind::Else:
    return "Else";
  case TokenKind::While:
    return "While";
  case TokenKind::Return:
    return "Return";
  case TokenKind::Const:
    return "Const";
  case TokenKind::Define:
    return "Define";
  case TokenKind::Plus:
    return "Plus";
  case TokenKind::Minus:
    return "Minus";
  case TokenKind::Multiply:
    return "Multiply";
  case TokenKind::Divide:
    return "Divide";
  case TokenKind::Modulo:
    return "Modulo";
  case TokenKind::Equals:
    return "Equals";
  case TokenKind::EqualEqual:
    return "EqualEqual";
  case TokenKind::Less:
    return "Less";
  case TokenKind::Greater:
    return "Greater";
  case TokenKind::Not:
    return "Not";
  case TokenKind::And:
    return "And";
  case TokenKind::Or:
    return "Or";
  case TokenKind::EndOfFile:
    return "EndOfFile";
  case TokenKind::Unknown:
    return "Unknown";
  default:
    return "???";
  }
}

void output_lex(basic_ofstream<char> &outputFile, std::vector<Token> tokens) {

  for (size_t i = 0; i < tokens.size(); i++) {
    const Token &tok = tokens[i];
    outputFile << "Token " << i << ":\n";
    outputFile << "  Kind: " << tokenKindToString(tok.kind) << "\n";
    outputFile << "  Lexeme: \"" << tok.lexeme << "\"\n";
    outputFile << "  Line: " << tok.pos.line << "\n";

    if (holds_alternative<int>(tok.literal)) {
      outputFile << "  Literal (int): " << get<int>(tok.literal) << "\n";
    } else if (holds_alternative<bool>(tok.literal)) {
      outputFile << "  Literal (bool): "
                 << (get<bool>(tok.literal) ? "true" : "false") << "\n";
    } else if (holds_alternative<string>(tok.literal)) {
      outputFile << "  Literal (string): \"" << get<string>(tok.literal)
                 << "\"\n";
    }

    outputFile << "\n";
  }
}
