#include "src/lexer.hpp"
#include "src/token.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

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

int main() {
  ifstream inputFile("test_input.txt");
  if (!inputFile) {
    cerr << "Error: Could not open test_input.txt" << endl;
    return 1;
  }

  stringstream buffer;
  buffer << inputFile.rdbuf();
  string sourceCode = buffer.str();
  inputFile.close();

  vector<Token> tokens = lex(sourceCode);

  ofstream outputFile("test_output.txt");
  if (!outputFile) {
    cerr << "Error: Could not open test_output.txt for writing" << endl;
    return 1;
  }

  outputFile << "Total tokens: " << tokens.size() << "\n\n";

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

  outputFile.close();
  cout << "Lexing complete! Output written to test_output.txt" << endl;
  cout << "Total tokens generated: " << tokens.size() << endl;

  return 0;
}
