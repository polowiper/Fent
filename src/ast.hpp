#pragma once
#include "token.hpp"
#include <memory>
#include <string>
#include <variant>
#include <vector>
class Expr;
class Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

class Expr {
public:
  virtual ~Expr() = default;
};

class LiteralExpr : public Expr {
public:
  std::variant<int, bool, std::string> value;

  explicit LiteralExpr(std::variant<int, bool, std::string> val)
      : value(std::move(val)) {}
};

class IdentifierExpr : public Expr {
public:
  std::string name;

  explicit IdentifierExpr(std::string n) : name(std::move(n)) {}
};

class BinaryExpr : public Expr {
public:
  std::string op; // "+", "-", "*", "/", "%", "==", "<", ">"
  ExprPtr left;
  ExprPtr right;

  BinaryExpr(std::string operation, ExprPtr l, ExprPtr r)
      : op(std::move(operation)), left(std::move(l)), right(std::move(r)) {}
};

class UnaryExpr : public Expr {
public:
  std::string op; // "-", "!"
  ExprPtr operand;

  UnaryExpr(std::string operation, ExprPtr expr)
      : op(std::move(operation)), operand(std::move(expr)) {}
};

// foo(a, b, c)
class CallExpr : public Expr {
public:
  std::string function;
  std::vector<ExprPtr> arguments;

  CallExpr(std::string func, std::vector<ExprPtr> args)
      : function(std::move(func)), arguments(std::move(args)) {}
};

class Stmt {
public:
  virtual ~Stmt() = default;
};

// x + 5;
class ExprStmt : public Stmt {
public:
  ExprPtr expression;

  explicit ExprStmt(ExprPtr expr) : expression(std::move(expr)) {}
};

// var x = 10;
class VarDeclStmt : public Stmt {
public:
  std::string name;
  ExprPtr initializer;
  bool isConst;

  VarDeclStmt(std::string n, ExprPtr init, bool constant = false)
      : name(std::move(n)), initializer(std::move(init)), isConst(constant) {}
};

// x = 42;
class AssignStmt : public Stmt {
public:
  std::string name;
  ExprPtr value;

  AssignStmt(std::string n, ExprPtr val)
      : name(std::move(n)), value(std::move(val)) {}
};

//{ stmt1; stmt2; ... }
class BlockStmt : public Stmt {
public:
  std::vector<StmtPtr> statements;

  explicit BlockStmt(std::vector<StmtPtr> stmts)
      : statements(std::move(stmts)) {}
};

// if (condition) thenBranch else elseBranch
class IfStmt : public Stmt {
public:
  ExprPtr condition;
  StmtPtr thenBranch;
  StmtPtr elseBranch; // can be nullptr

  IfStmt(ExprPtr cond, StmtPtr thenBr, StmtPtr elseBr = nullptr)
      : condition(std::move(cond)), thenBranch(std::move(thenBr)),
        elseBranch(std::move(elseBr)) {}
};

// while (condition) body
class WhileStmt : public Stmt {
public:
  ExprPtr condition;
  StmtPtr body;

  WhileStmt(ExprPtr cond, StmtPtr b)
      : condition(std::move(cond)), body(std::move(b)) {}
};

// Return statement: return expr;
class ReturnStmt : public Stmt {
public:
  ExprPtr value; // can be nullptr for void return

  explicit ReturnStmt(ExprPtr val = nullptr) : value(std::move(val)) {}
};

struct FunctionParam {
  std::string name;
  bool isConst;

  FunctionParam(std::string n, bool constant = true)
      : name(std::move(n)), isConst(constant) {}
};

// Function definition: define foo(a, b) { body }
// Or with mutable params: define foo(var a, var b) { body }
class FunctionDef : public Stmt {
public:
  std::string name;
  std::vector<FunctionParam> parameters;
  StmtPtr body;

  FunctionDef(std::string n, std::vector<FunctionParam> params, StmtPtr b)
      : name(std::move(n)), parameters(std::move(params)), body(std::move(b)) {}
};

class Program {
public:
  static std::vector<StmtPtr> tokens_to_ast(const std::vector<Token> &tokens);
  std::vector<StmtPtr> statements;

  explicit Program(std::vector<StmtPtr> stmts) : statements(std::move(stmts)) {}
};

void printExpr(std::ostream &out, const Expr *expr, int indent);
void printStmt(std::ostream &out, const Stmt *stmt, int indent);
