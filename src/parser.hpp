#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <vector>

ExprPtr parse(const std::vector<Token> &tokens);
