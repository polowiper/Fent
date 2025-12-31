#pragma once
#include "token.hpp"
#include <fstream>
#include <vector>
using namespace std;
vector<Token> lex(string_view SourceCode);
void output_lex(basic_ofstream<char> &outputFile, vector<Token> tokens);
