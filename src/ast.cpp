#include "ast.hpp"
#include <iostream>
#include <stdexcept>

#define INDENT_LEVEL 2
class Parser {
private:
  const std::vector<Token> &tokens;
  size_t current = 0;

  bool isAtEnd() const { return peek().kind == TokenKind::EndOfFile; }

  const Token &peek() const { return tokens[current]; }

  const Token &previous() const { return tokens[current - 1]; }

  const Token &advance() {
    if (!isAtEnd())
      current++;
    return previous();
  }

  bool check(TokenKind kind) const {
    if (isAtEnd())
      return false;
    return peek().kind == kind;
  }

  bool match(TokenKind kind) {
    if (check(kind)) {
      advance();
      return true;
    }
    return false;
  }

  void expect(TokenKind kind, const std::string &message) {
    if (check(kind)) {
      advance();
      return;
    }
    throw std::runtime_error(message + " at line " +
                             std::to_string(peek().pos.line));
  }

public:
  explicit Parser(const std::vector<Token> &toks) : tokens(toks) {}

  std::vector<StmtPtr> parse() {
    std::vector<StmtPtr> statements;
    while (!isAtEnd()) {
      statements.push_back(parseStatement());
    }
    return statements;
  }

  StmtPtr parseStatement() {
    if (match(TokenKind::Const))
      return parseVarDecl();
    if (match(TokenKind::If))
      return parseIfStmt();
    if (match(TokenKind::While))
      return parseWhileStmt();
    if (match(TokenKind::Return))
      return parseReturnStmt();
    if (match(TokenKind::LBrace))
      return parseBlockStmt();

    if (check(TokenKind::Identifier)) {
      size_t saved = current;
      advance(); // consume identifier
      if (match(TokenKind::Equals)) {
        std::string name = tokens[saved].lexeme;
        ExprPtr value = parseExpression();
        expect(TokenKind::Semicolon, "Expected ';' after assignment");
        return std::make_unique<AssignStmt>(name, std::move(value));
      }
      current = saved; // backtrack
    }

    return parseExprStmt();
  }

  // const x = expression;
  StmtPtr parseVarDecl() {
    expect(TokenKind::Identifier, "Expected variable name");
    std::string name = previous().lexeme;

    expect(TokenKind::Equals, "Expected '=' after variable name");
    ExprPtr initializer = parseExpression();
    expect(TokenKind::Semicolon, "Expected ';' after variable declaration");

    return std::make_unique<VarDeclStmt>(name, std::move(initializer), true);
  }

  // if (condition) statement [else statement]
  StmtPtr parseIfStmt() {
    expect(TokenKind::Lpar, "Expected '(' after 'if'");
    ExprPtr condition = parseExpression();
    expect(TokenKind::Rpar, "Expected ')' after if condition");

    StmtPtr thenBranch = parseStatement();
    StmtPtr elseBranch = nullptr;

    if (match(TokenKind::Else)) {
      elseBranch = parseStatement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch),
                                    std::move(elseBranch));
  }

  // while (condition) statement
  StmtPtr parseWhileStmt() {
    expect(TokenKind::Lpar, "Expected '(' after 'while'");
    ExprPtr condition = parseExpression();
    expect(TokenKind::Rpar, "Expected ')' after while condition");

    StmtPtr body = parseStatement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
  }

  // return [expression];
  StmtPtr parseReturnStmt() {
    ExprPtr value = nullptr;

    if (!check(TokenKind::Semicolon)) {
      value = parseExpression();
    }

    expect(TokenKind::Semicolon, "Expected ';' after return statement");
    return std::make_unique<ReturnStmt>(std::move(value));
  }

  // { statement* }
  StmtPtr parseBlockStmt() {
    std::vector<StmtPtr> statements;

    while (!check(TokenKind::RBrace) && !isAtEnd()) {
      statements.push_back(parseStatement());
    }

    expect(TokenKind::RBrace, "Expected '}' after block");
    return std::make_unique<BlockStmt>(std::move(statements));
  }

  // expression;
  StmtPtr parseExprStmt() {
    ExprPtr expr = parseExpression();
    expect(TokenKind::Semicolon, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
  }

  ExprPtr parseExpression() { return parseEquality(); }

  ExprPtr parseEquality() {
    ExprPtr expr = parseComparison();

    while (match(TokenKind::EqualEqual)) {
      std::string op = previous().lexeme;
      ExprPtr right = parseComparison();
      expr =
          std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
  }

  // < >
  ExprPtr parseComparison() {
    ExprPtr expr = parseTerm();

    while (check(TokenKind::Less) || check(TokenKind::Greater)) {
      advance();
      std::string op = previous().lexeme;
      ExprPtr right = parseTerm();
      expr =
          std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
  }

  // + -
  ExprPtr parseTerm() {
    ExprPtr expr = parseFactor();

    while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
      advance();
      std::string op = previous().lexeme;
      ExprPtr right = parseFactor();
      expr =
          std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
  }

  // * / %
  ExprPtr parseFactor() {
    ExprPtr expr = parseUnary();

    while (check(TokenKind::Multiply) || check(TokenKind::Divide) ||
           check(TokenKind::Modulo)) {
      advance();
      std::string op = previous().lexeme;
      ExprPtr right = parseUnary();
      expr =
          std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
  }

  // - !
  ExprPtr parseUnary() {
    if (check(TokenKind::Minus) || check(TokenKind::Not)) {
      advance();
      std::string op = previous().lexeme;
      ExprPtr right = parseUnary();
      return std::make_unique<UnaryExpr>(op, std::move(right));
    }

    return parsePrimary();
  }

  ExprPtr parsePrimary() {
    // Literals
    if (match(TokenKind::Number)) {
      int value = std::get<int>(previous().literal);
      return std::make_unique<LiteralExpr>(value);
    }

    if (match(TokenKind::String)) {
      std::string value = std::get<std::string>(previous().literal);
      return std::make_unique<LiteralExpr>(value);
    }

    if (match(TokenKind::True)) {
      return std::make_unique<LiteralExpr>(true);
    }

    if (match(TokenKind::False)) {
      return std::make_unique<LiteralExpr>(false);
    }

    // Identifier or function call
    if (match(TokenKind::Identifier)) {
      std::string name = previous().lexeme;

      // Check for function call
      if (match(TokenKind::Lpar)) {
        std::vector<ExprPtr> args;

        if (!check(TokenKind::Rpar)) {
          do {
            args.push_back(parseExpression());
          } while (match(TokenKind::Comma));
        }

        expect(TokenKind::Rpar, "Expected ')' after arguments");
        return std::make_unique<CallExpr>(name, std::move(args));
      }

      return std::make_unique<IdentifierExpr>(name);
    }

    // Parenthesized expression
    if (match(TokenKind::Lpar)) {
      ExprPtr expr = parseExpression();
      expect(TokenKind::Rpar, "Expected ')' after expression");
      return expr;
    }

    throw std::runtime_error("Expected expression at line " +
                             std::to_string(peek().pos.line));
  }
};

std::vector<StmtPtr> Program::tokens_to_ast(const std::vector<Token> &tokens) {
  Parser parser(tokens);
  return parser.parse();
}

std::string getIndent(int level) {
  return std::string(level * INDENT_LEVEL, ' ');
}

void printExpr(std::ostream &out, const Expr *expr, int indent) {
  std::string ind = getIndent(indent);

  if (auto *lit = dynamic_cast<const LiteralExpr *>(expr)) {
    out << ind << "LiteralExpr: ";
    if (std::holds_alternative<int>(lit->value)) {
      out << std::get<int>(lit->value);
    } else if (std::holds_alternative<bool>(lit->value)) {
      out << (std::get<bool>(lit->value) ? "true" : "false");
    } else if (std::holds_alternative<std::string>(lit->value)) {
      out << "\"" << std::get<std::string>(lit->value) << "\"";
    }
    out << "\n";
  } else if (auto *id = dynamic_cast<const IdentifierExpr *>(expr)) {
    out << ind << "IdentifierExpr: " << id->name << "\n";
  } else if (auto *bin = dynamic_cast<const BinaryExpr *>(expr)) {
    out << ind << "BinaryExpr: " << bin->op << "\n";
    out << ind << "  Left:\n";
    printExpr(out, bin->left.get(), indent + INDENT_LEVEL);
    out << ind << "  Right:\n";
    printExpr(out, bin->right.get(), indent + INDENT_LEVEL);
  } else if (auto *un = dynamic_cast<const UnaryExpr *>(expr)) {
    out << ind << "UnaryExpr: " << un->op << "\n";
    out << ind << "  Operand:\n";
    printExpr(out, un->operand.get(), indent + INDENT_LEVEL);
  } else if (auto *call = dynamic_cast<const CallExpr *>(expr)) {
    out << ind << "CallExpr: " << call->function << "\n";
    out << ind << "  Arguments (" << call->arguments.size() << "):\n";
    for (const auto &arg : call->arguments) {
      printExpr(out, arg.get(), indent + INDENT_LEVEL);
    }
  } else {
    out << ind << "UnknownExpr\n";
  }
}

void printStmt(std::ostream &out, const Stmt *stmt, int indent) {
  std::string ind = getIndent(indent);

  if (auto *expr = dynamic_cast<const ExprStmt *>(stmt)) {
    out << ind << "ExprStmt:\n";
    printExpr(out, expr->expression.get(), indent + INDENT_LEVEL / 2);
  } else if (auto *varDecl = dynamic_cast<const VarDeclStmt *>(stmt)) {
    out << ind << "VarDeclStmt: " << varDecl->name
        << (varDecl->isConst ? " (const)" : "") << "\n";
    out << ind << "  Initializer:\n";
    printExpr(out, varDecl->initializer.get(), indent + INDENT_LEVEL);
  } else if (auto *assign = dynamic_cast<const AssignStmt *>(stmt)) {
    out << ind << "AssignStmt: " << assign->name << "\n";
    out << ind << "  Value:\n";
    printExpr(out, assign->value.get(), indent + INDENT_LEVEL);
  } else if (auto *block = dynamic_cast<const BlockStmt *>(stmt)) {
    out << ind << "BlockStmt (" << block->statements.size()
        << " statements):\n";
    for (const auto &s : block->statements) {
      printStmt(out, s.get(), indent + INDENT_LEVEL / 2);
    }
  } else if (auto *ifStmt = dynamic_cast<const IfStmt *>(stmt)) {
    out << ind << "IfStmt:\n";
    out << ind << "  Condition:\n";
    printExpr(out, ifStmt->condition.get(), indent + INDENT_LEVEL);
    out << ind << "  Then:\n";
    printStmt(out, ifStmt->thenBranch.get(), indent + INDENT_LEVEL);
    if (ifStmt->elseBranch) {
      out << ind << "  Else:\n";
      printStmt(out, ifStmt->elseBranch.get(), indent + INDENT_LEVEL);
    }
  } else if (auto *whileStmt = dynamic_cast<const WhileStmt *>(stmt)) {
    out << ind << "WhileStmt:\n";
    out << ind << "  Cond:\n";
    printExpr(out, whileStmt->condition.get(), indent + INDENT_LEVEL);
    out << ind << "  Body:\n";
    printStmt(out, whileStmt->body.get(), indent + INDENT_LEVEL);
  } else if (auto *ret = dynamic_cast<const ReturnStmt *>(stmt)) {
    out << ind << "ReturnStmt:\n";
    if (ret->value) {
      out << ind << "  Value:\n";
      printExpr(out, ret->value.get(), indent + INDENT_LEVEL);
    } else {
      out << ind << "  NULL (no value assigned)\n";
    }
  } else {
    out << ind << "UnknownStmt\n";
  }
}
