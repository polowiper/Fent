#include "src/ast.hpp"
#include "src/code_gen.hpp"
#include "src/lexer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

using namespace std;

void print_usage(const char *program_name) {
  cerr << "Usage: " << program_name << " <input.fent> [-o <output.asm>]"
       << endl;
  cerr << "  Compiles fent source code to x86_64 NASM assembly" << endl;
  cerr << endl;
  cerr << "Options:" << endl;
  cerr << "  -o <file>    Specify output file (default: output.asm)" << endl;
  cerr << "  -l, --lexer  Shows the token list as tokens_<file>.txt" << endl;
  cerr << "  -a, --ast    Shows ast output file as ast_<file>.txt" << endl;
  cerr << "  -h, --help   Show this help message" << endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }
  bool lexer_debug = false;
  bool ast_debug = false;
  string input_file;
  string output_file = "output.asm";

  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-o") {
      if (i + 1 < argc) {
        output_file = argv[++i];
      } else {
        cerr << "Error: -o requires an argument" << endl;
        return 1;
      }
    } else if (arg == "-a" || arg == "--ast") {
      ast_debug = true;
    } else if (arg == "-l" || arg == "--lexer") {
      lexer_debug = true;
    } else {
      input_file = arg;
    }
  }

  if (input_file.empty()) {
    cerr << "Error: No input file specified" << endl;
    print_usage(argv[0]);
    return 1;
  }

  ifstream inputFileStream(input_file);
  if (!inputFileStream) {
    cerr << "Error: Could not open input file: " << input_file << endl;
    return 1;
  }

  stringstream buffer;
  buffer << inputFileStream.rdbuf();
  string sourceCode = buffer.str();
  inputFileStream.close();

  // Lexical analysis
  vector<Token> tokens;
  try {
    tokens = lex(sourceCode);
  } catch (const exception &e) {
    cerr << "Lexer error: " << e.what() << endl;
    return 1;
  }

  // Output lexer debug info if requested
  if (lexer_debug) {
    string base_name = filesystem::path(input_file).filename().string();
    string lexer_out = "tokens_" + base_name + ".txt";
    ofstream lexerFileStream(lexer_out);
    if (!lexerFileStream) {
      cerr << "Error: Could not open lexer output file: " << lexer_out << endl;
      return 1;
    }
    output_lex(lexerFileStream, tokens);
    lexerFileStream.close();
    cout << "Lexer output written to: " << lexer_out << endl;
  }

  // Parse to AST
  vector<StmtPtr> ast;
  try {
    ast = Program::tokens_to_ast(tokens);
    cout << "Parsed " << ast.size() << " top-level statement(s)" << endl;
  } catch (const exception &e) {
    cerr << "Parse error: " << e.what() << endl;
    return 1;
  }

  // Output AST debug info if requested
  if (ast_debug) {
    string base_name = filesystem::path(input_file).filename().string();
    string ast_out = "ast_" + base_name + ".txt";
    ofstream astFileStream(ast_out);
    if (!astFileStream) {
      cerr << "Error: Could not open AST output file: " << ast_out << endl;
      return 1;
    }
    for (const auto &stmt : ast) {
      printStmt(astFileStream, stmt.get(), 0);
    }
    astFileStream.close();
    cout << "AST output written to: " << ast_out << endl;
  }

  // Generate code
  Program program(std::move(ast));
  ofstream outputFileStream(output_file);
  if (!outputFileStream) {
    cerr << "Error: Could not open output file: " << output_file << endl;
    return 1;
  }

  try {
    ast_to_bin(outputFileStream, program);
    outputFileStream.close();
    cout << "Assembly generated: " << output_file << endl;
  } catch (const exception &e) {
    cerr << "Codegen error: " << e.what() << endl;
    return 1;
  }

  return 0;
}
