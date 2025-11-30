#pragma once
#include <memory>
#include <string>
#include <variant>

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct BinaryOp {
  std::string op;
  ExprPtr left;
  ExprPtr right;
};

struct UnaryOp {
  std::string op;
  ExprPtr operand;
};

struct Identifier {
  std::string name;
};

struct Literal {
  std::variant<int, char, bool> value;
};

struct Expr {
  using Variant = std::variant<BinaryOp, UnaryOp, Identifier, Literal>;
  Variant node;

  template <class T> explicit Expr(T v) : node(std::move(v)) {}
};
