/*
This is the code that will be used to the CodeGen.hpp on
the x86_64 architecture, I might do RISC-V later on.
*/

/*
Internal representation of certain tokens:
Number -> 32 bits (int)
String -> null-terminated sequence of bytes
Boolean -> 1 byte (0 or 1) (hopefuly)
*/

#pragma once
#include "../ast.hpp"
#include <iostream>
#include <string>
using namespace std;

string HandleBinaryOp(const BinaryExpr *binExpr) {
  string result;
  result += "mov rbx";
  result += "pop rbx\n";
  if (binExpr->op == "+") {
    result += "add rbx, rax\n";
    result += "mov rax, rbx\n";
  } else if (binExpr->op == "-") {
    result += "sub rbx, rax\n";
    result += "mov rax, rbx\n";
  } else if (binExpr->op == "*") {
    result += "imul rbx, rax\n";
    result += "mov rax, rbx\n";
  } else if (binExpr->op == "/") {
    result += "mov rcx, rax\n";
    result += "mov rax, rbx\n";
    result += "cdq\n";
    result += "idiv rcx\n";
  } else if (binExpr->op == "%") {
    result += "mov rcx, rax\n";
    result += "mov rax, rbx\n";
    result += "cdq\n";
    result += "idiv rcx\n";
    result += "mov rax, rdx\n";
  }

  return result;
}

void AstToBin(std::ostream &out, const vector<StmtPtr> Statements) {
  for (int i = 0; i < Statements.size(); i++) {
    ;
  }
}
