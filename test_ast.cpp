#include "src/ast.hpp"
#include "src/lexer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int main(void) {
  ifstream inputFile("test_input.txt");
  if (!inputFile) {
    cerr << "Error: Could not open test_input.txt" << endl;
    return 1;
  }

  stringstream buffer;
  buffer << inputFile.rdbuf();
  string sourceCode = buffer.str();
  inputFile.close();

  // Lex the source code
  vector<Token> tokens = lex(sourceCode);

  // Parse into AST
  vector<StmtPtr> ast;
  try {
    ast = Program::tokens_to_ast(tokens);
    cout << "AST generated with " << ast.size() << " top-level statements."
         << endl;
  } catch (const exception &e) {
    cerr << "Parse error: " << e.what() << endl;
    return 1;
  }

  ofstream outputFile("test_ast_output.txt");
  if (!outputFile) {
    cerr << "Error: Could not open test_ast_output.txt for writing" << endl;
    return 1;
  }

  outputFile << "Total statements: " << ast.size() << "\n\n";

  for (size_t i = 0; i < ast.size(); i++) {
    outputFile << "Statement " << i << ":\n";
    printStmt(outputFile, ast[i].get(), 1);
    outputFile << "\n";
  }

  outputFile.close();
  cout << "AST output written to test_ast_output.txt" << endl;

  return 0;
}
