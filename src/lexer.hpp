#pragma once
#include "token.hpp"
#include <vector>

std::vector<Token> lex(std::string_view SourceCode);
