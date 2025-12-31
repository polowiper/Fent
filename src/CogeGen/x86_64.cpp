#include "../ast.hpp"
#include "../code_gen.hpp"
#include <functional>
#include <memory>
#include <string>
#include <variant>
using namespace std;

#define u16 uint16_t
#define u32 uint32_t

enum class VarType { INT, BOOL, STRING };

typedef struct {
  u16 rbp_offset;
  u16 size;
  string name;
  void *value;
  VarType type;
  string string_label; // If type==STRING, stores the label of the string
  bool is_param; // True if this is a function parameter (accessed as [rbp +
                 // offset])
} Variable;

struct StringData {
  string label; // Unique label for this string (e.g., "str_0")
  string value; // content
  size_t length;
  bool is_computed; // True if from concatenation

  StringData(string lbl, string val, bool computed = false)
      : label(lbl), value(val), length(val.length()), is_computed(computed) {}
};

struct Data_table {
  vector<StringData> strings;
  u32 string_counter = 0;

  string add_string(const string &value, bool is_computed = false) {
    string label = "str_" + to_string(string_counter++);
    strings.emplace_back(label, value, is_computed);
    return label;
  }

  const StringData *find_string(const string &label) const {
    for (const auto &s : strings) {
      if (s.label == label)
        return &s;
    }
    return nullptr;
  }
};

struct Var_table {
  vector<Variable> table;
  void *rbp_ptr;
};

struct CodegenFunctionParam {
  string name;
  VarType type;
  bool isConst;
};

struct FunctionInfo {
  string name;
  string label; // Assembly label for the function
  vector<CodegenFunctionParam> parameters;
  VarType return_type; // Return type (Int by default for now)

  FunctionInfo(string n, string lbl, vector<CodegenFunctionParam> params,
               VarType ret_type = VarType::INT)
      : name(n), label(lbl), parameters(params), return_type(ret_type) {}
};

struct Function_table {
  vector<FunctionInfo> functions;

  void add_function(const string &name, const string &label,
                    const vector<CodegenFunctionParam> &params,
                    VarType return_type = VarType::INT) {
    functions.emplace_back(name, label, params, return_type);
  }

  const FunctionInfo *find_function(const string &name) const {
    for (const auto &func : functions) {
      if (func.name == name)
        return &func;
    }
    return nullptr;
  }
};

struct CodegenContext {
  u32 label_counter = 0;
  u32 function_counter = 0;
  bool in_function = false; // Track if we're currently in a function (vs main)

  string generate_label(const string &prefix) {
    return prefix + "_" + to_string(label_counter++);
  }

  string generate_function_label(const string &func_name) {
    return "func_" + func_name;
  }
};

string generate_asm_headers() {
  string header;
  header += "global _start\n\nsection .text\n_start:\n";
  return header;
}

VarType get_expr_type(Expr *expr, const Var_table &var_table) {
  if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
    if (holds_alternative<int>(lit->value))
      return VarType::INT;
    if (holds_alternative<bool>(lit->value))
      return VarType::BOOL;
    if (holds_alternative<string>(lit->value))
      return VarType::STRING;
  } else if (auto ident = dynamic_cast<IdentifierExpr *>(expr)) {
    // Lmfaoo sorry I couldn't come up with a better idea
    for (const auto &var : var_table.table) {
      if (var.name == ident->name) {
        return var.type;
      }
    }
  }
  return VarType::INT;
}

bool get_string_label(Expr *expr, const Var_table &var_table,
                      string &out_label) {
  if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
    if (holds_alternative<string>(lit->value)) {
      // This is handled in handle_value, but we need the label
      // We'll return false here and handle it differently
      return false;
    }
  } else if (auto ident = dynamic_cast<IdentifierExpr *>(expr)) {
    for (const auto &var : var_table.table) {
      if (var.name == ident->name && var.type == VarType::STRING) {
        out_label = var.string_label;
        return true;
      }
    }
  }
  return false;
}

void handle_value(string &out, LiteralExpr *v, Var_table &var_table,
                  Data_table &data_table, string *out_label = nullptr) {
  if (holds_alternative<int>(v->value)) {
    out += "  mov rax, " + to_string(get<int>(v->value)) + "\n";
  } else if (holds_alternative<bool>(v->value)) {
    bool val = get<bool>(v->value);
    out += "  mov rax, " + string(val ? "1" : "0") + "\n";
  } else if (holds_alternative<string>(v->value)) {
    // Add string to data table and get its label
    string label = data_table.add_string(get<string>(v->value), false);

    // If caller wants the label, store it
    if (out_label) {
      *out_label = label;
    }

    // Load address of string into rax
    out += "  lea rax, [rel " + label + "]\n";
  }
}
void handle_expr(string &out, Expr *expr, Var_table &var_table,
                 Data_table &data_table, Function_table &func_table,
                 string *result_label);

void handle_un_expr(string &out, UnaryExpr *u, Var_table &var_table,
                    Data_table &data_table, Function_table &func_table) {
  if (auto lit = dynamic_cast<LiteralExpr *>(u->operand.get())) {
    handle_value(out, lit, var_table, data_table);
  } else {
    handle_expr(out, u->operand.get(), var_table, data_table, func_table,
                nullptr);
  }

  if (u->op == "-") {
    out += "  neg rax\n";
  } else if (u->op == "!") {
    // Logical NOT: if rax == 0, set to 1, else set to 0
    out += "  test rax, rax\n";
    out += "  sete al\n";
    out += "  movzx rax, al\n";
  }
}

void handle_bin_expr(string &out, BinaryExpr *b, Var_table &var_table,
                     Data_table &data_table, Function_table &func_table,
                     string *result_label = nullptr) {
  VarType left_type = get_expr_type(b->left.get(), var_table);
  VarType right_type = get_expr_type(b->right.get(), var_table);

  if (b->op == "+" &&
      (left_type == VarType::STRING || right_type == VarType::STRING)) {
    // compile-time concatenation
    string left_str_val, right_str_val;
    bool left_is_known = false, right_is_known = false;

    if (auto lit = dynamic_cast<LiteralExpr *>(b->left.get())) {
      if (holds_alternative<string>(lit->value)) {
        left_str_val = get<string>(lit->value);
        left_is_known = true;
      }
    } else if (auto ident = dynamic_cast<IdentifierExpr *>(b->left.get())) {
      for (const auto &var : var_table.table) {
        if (var.name == ident->name && var.type == VarType::STRING) {
          // Look up the string value from data table using the variable's label
          const StringData *str_data = data_table.find_string(var.string_label);
          if (str_data) {
            left_str_val = str_data->value;
            left_is_known = true;
          }
          break;
        }
      }
    } else if (auto bin = dynamic_cast<BinaryExpr *>(b->left.get())) {
      // Handle nested concatenation recursively
      string nested_label;
      string nested_code;
      handle_bin_expr(nested_code, bin, var_table, data_table, func_table,
                      &nested_label);
      if (!nested_label.empty()) {
        const StringData *str_data = data_table.find_string(nested_label);
        if (str_data) {
          left_str_val = str_data->value;
          left_is_known = true;
        }
      }
    }

    if (auto lit = dynamic_cast<LiteralExpr *>(b->right.get())) {
      if (holds_alternative<string>(lit->value)) {
        right_str_val = get<string>(lit->value);
        right_is_known = true;
      }
    } else if (auto ident = dynamic_cast<IdentifierExpr *>(b->right.get())) {
      for (const auto &var : var_table.table) {
        if (var.name == ident->name && var.type == VarType::STRING) {
          const StringData *str_data = data_table.find_string(var.string_label);
          if (str_data) {
            right_str_val = str_data->value;
            right_is_known = true;
          }
          break;
        }
      }
    } else if (auto bin = dynamic_cast<BinaryExpr *>(b->right.get())) {
      string nested_label;
      string nested_code;
      handle_bin_expr(nested_code, bin, var_table, data_table, func_table,
                      &nested_label);
      if (!nested_label.empty()) {
        const StringData *str_data = data_table.find_string(nested_label);
        if (str_data) {
          right_str_val = str_data->value;
          right_is_known = true;
        }
      }
    }

    // If both values are known at compile time, concatenate them
    if (left_is_known && right_is_known) {
      string concatenated = left_str_val + right_str_val;
      string label = data_table.add_string(concatenated, true);

      if (result_label) {
        *result_label = label;
      }

      out += "  lea rax, [rel " + label + "]\n";
      return;
    }

    // Otherwise we're fucking cooked lmao don't wanna implement that at runtime
    out += "  ; ERROR: Runtime string concatenation not yet implemented\n";
    out += "  xor rax, rax\n";
    return;
  }

  // Evaluate left expression and store in rax
  if (auto lit = dynamic_cast<LiteralExpr *>(b->left.get())) {
    handle_value(out, lit, var_table, data_table);
  } else {
    handle_expr(out, b->left.get(), var_table, data_table, func_table, nullptr);
  }

  // Save left value to stack
  out += "  push rax\n";

  // Evaluate right expression and store in rax
  if (auto lit = dynamic_cast<LiteralExpr *>(b->right.get())) {
    handle_value(out, lit, var_table, data_table);
  } else {
    handle_expr(out, b->right.get(), var_table, data_table, func_table,
                nullptr);
  }

  // Pop left value into rbx
  out += "  pop rbx\n";

  // Perform operation (left is in rbx, right is in rax)
  if (b->op == "+") {
    out += "  add rax, rbx\n";
  } else if (b->op == "-") {
    out += "  sub rbx, rax\n";
    out += "  mov rax, rbx\n";
  } else if (b->op == "*") {
    out += "  imul rax, rbx\n";
  } else if (b->op == "/") {
    out += "  mov rdx, 0\n";
    out += "  mov rcx, rax\n";
    out += "  mov rax, rbx\n";
    out += "  idiv rcx\n";
  } else if (b->op == "%") {
    out += "  mov rdx, 0\n";
    out += "  mov rcx, rax\n";
    out += "  mov rax, rbx\n";
    out += "  idiv rcx\n";
    out += "  mov rax, rdx\n";
  } else if (b->op == "==") {
    out += "  cmp rbx, rax\n";
    out += "  sete al\n";       // Set AL to 1 if equal, 0 otherwise
    out += "  movzx rax, al\n"; // Zero-extend AL to RAX
  } else if (b->op == "<") {
    out += "  cmp rbx, rax\n";
    out += "  setl al\n"; // Set AL to 1 if rbx < rax
    out += "  movzx rax, al\n";
  } else if (b->op == ">") {
    out += "  cmp rbx, rax\n";
    out += "  setg al\n"; // Set AL to 1 if rbx > rax
    out += "  movzx rax, al\n";
  }
  // Result is in rax
}

void handle_expr(string &out, Expr *expr, Var_table &var_table,
                 Data_table &data_table, Function_table &func_table,
                 string *result_label = nullptr) {

  if (auto bin = dynamic_cast<BinaryExpr *>(expr)) {
    handle_bin_expr(out, bin, var_table, data_table, func_table, result_label);
  }

  else if (auto un = dynamic_cast<UnaryExpr *>(expr)) {
    handle_un_expr(out, un, var_table, data_table, func_table);
  }

  else if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
    handle_value(out, lit, var_table, data_table, result_label);
  }

  // Function calls
  else if (auto call = dynamic_cast<CallExpr *>(expr)) {
    // This will be changed later on as I plan to add more builtin functions and
    // a syscall function
    if (call->function == "print") {
      // Assumes the argument is a string
      if (!call->arguments.empty()) {
        // Evaluate the argument (should be a string pointer in rax)
        handle_expr(out, call->arguments[0].get(), var_table, data_table,
                    func_table, nullptr);

        // Calculate string length (assume null-terminated)
        // For simplicity, we'll use a fixed approach or look up the length
        // Store string pointer
        out += "  mov rsi, rax\n"; // String pointer in rsi

        out += "  mov rdi, rax\n"; // Copy pointer
        out += "  xor rcx, rcx\n"; // Counter = 0
        out += "  dec rcx\n";      // rcx = -1 (for repne)
        out += "  xor al, al\n";   // Search for 0
        out += "  repne scasb\n";  // Scan until we find 0
        out += "  not rcx\n";      // Invert rcx
        out += "  dec rcx\n";      // Subtract 1 (length without null)
        out += "  mov rdx, rcx\n"; // Length in rdx

        // write(1, rsi, rdx)
        out += "  mov rax, 1\n"; // syscall number for write
        out += "  mov rdi, 1\n"; // file descriptor 1 (stdout)
        out += "  syscall\n";

        // Return 0 in rax
        out += "  xor rax, rax\n";
      }
    } else {
      // Look up user-defined function
      const FunctionInfo *func_info = func_table.find_function(call->function);
      if (func_info) {
        // User-defined function call
        // Push arguments onto stack in RIGHT-TO-LEFT order (C calling
        // convention)
        for (auto it = call->arguments.rbegin(); it != call->arguments.rend();
             ++it) {
          handle_expr(out, it->get(), var_table, data_table, func_table,
                      nullptr);
          out += "  push rax\n"; // Push argument onto stack
        }

        // Call the function
        out += "  call " + func_info->label + "\n";

        // Clean up the stack (caller cleans up)
        if (!call->arguments.empty()) {
          out += "  add rsp, " + to_string(call->arguments.size() * 8) + "\n";
        }

        // Return value is now in rax
      } else {
        // Unknown function
        out += "  ; ERROR: Unknown function call: " + call->function + "\n";
        out += "  xor rax, rax\n";
      }
    }
  } else if (auto ident = dynamic_cast<IdentifierExpr *>(expr)) {
    // Look up variable in var_table
    for (const auto &var : var_table.table) {
      if (var.name == ident->name) {
        if (var.is_param) {
          out += "  mov rax, [rbp + " + std::to_string(var.rbp_offset) + "]\n";
        } else {
          out += "  mov rax, [rbp - " + std::to_string(var.rbp_offset) + "]\n";
        }
        if (result_label && var.type == VarType::STRING) {
          *result_label = var.string_label;
        }
        return;
      }
    }
  }
}

void handle_stmt(string &out, Stmt *stmt, Var_table &var_table,
                 Data_table &data_table, Function_table &func_table,
                 CodegenContext &ctx);

void handle_expr_stmt(string &out, ExprStmt *s, Var_table &var_table,
                      Data_table &data_table, Function_table &func_table) {
  handle_expr(out, s->expression.get(), var_table, data_table, func_table,
              nullptr);
}

void handle_stmt(string &out, Stmt *stmt, Var_table &var_table,
                 Data_table &data_table, Function_table &func_table,
                 CodegenContext &ctx) {
  if (auto expr_stmt = dynamic_cast<ExprStmt *>(stmt)) {
    handle_expr_stmt(out, expr_stmt, var_table, data_table, func_table);
  } else if (auto var_decl = dynamic_cast<VarDeclStmt *>(stmt)) {
    if (var_decl->initializer) {
      string result_label;
      handle_expr(out, var_decl->initializer.get(), var_table, data_table,
                  func_table, &result_label);

      // Add variable to table FIRST to calculate correct offset
      // Count only local variables (non-parameters) for offset calculation
      int local_var_count = 0;
      for (const auto &v : var_table.table) {
        if (!v.is_param)
          local_var_count++;
      }

      Variable var;
      var.name = var_decl->name;
      var.rbp_offset = (local_var_count + 1) * 8;
      var.size = 8; // Assuming 64-bit values for now
      var.value = nullptr;
      var.type = get_expr_type(var_decl->initializer.get(), var_table);
      var.string_label = result_label;
      var.is_param = false;
      var_table.table.push_back(var);

      // Store result at fixed offset (don't use push as it's affected by rsp
      // changes)
      out += "  mov [rbp - " + std::to_string(var.rbp_offset) + "], rax\n";
    }
  } else if (auto assign = dynamic_cast<AssignStmt *>(stmt)) {
    handle_expr(out, assign->value.get(), var_table, data_table, func_table,
                nullptr);

    for (const auto &var : var_table.table) {
      if (var.name == assign->name) {
        if (var.is_param) {
          out += "  mov [rbp + " + std::to_string(var.rbp_offset) + "], rax\n";
        } else {
          out += "  mov [rbp - " + std::to_string(var.rbp_offset) + "], rax\n";
        }
        return;
      }
    }
  } else if (auto block = dynamic_cast<BlockStmt *>(stmt)) {
    for (const auto &s : block->statements) {
      handle_stmt(out, s.get(), var_table, data_table, func_table, ctx);
    }
  } else if (auto if_stmt = dynamic_cast<IfStmt *>(stmt)) {
    string else_label = ctx.generate_label("else");
    string end_label = ctx.generate_label("endif");

    handle_expr(out, if_stmt->condition.get(), var_table, data_table,
                func_table, nullptr);

    out += "  test rax, rax\n";
    if (if_stmt->elseBranch) {
      out += "  jz " + else_label + "\n";
    } else {
      out += "  jz " + end_label + "\n";
    }

    handle_stmt(out, if_stmt->thenBranch.get(), var_table, data_table,
                func_table, ctx);

    if (if_stmt->elseBranch) {
      out += "  jmp " + end_label + "\n";
      out += else_label + ":\n";
      handle_stmt(out, if_stmt->elseBranch.get(), var_table, data_table,
                  func_table, ctx);
    }

    out += end_label + ":\n";
  } else if (auto while_stmt = dynamic_cast<WhileStmt *>(stmt)) {
    string loop_start = ctx.generate_label("while_start");
    string loop_end = ctx.generate_label("while_end");

    // Loop start
    out += loop_start + ":\n";

    // Evaluate condition
    handle_expr(out, while_stmt->condition.get(), var_table, data_table,
                func_table, nullptr);

    // Test if condition is false (0)
    out += "  test rax, rax\n";
    out += "  jz " + loop_end + "\n";

    // Loop body
    handle_stmt(out, while_stmt->body.get(), var_table, data_table, func_table,
                ctx);

    // Jump back to start
    out += "  jmp " + loop_start + "\n";

    // Loop end
    out += loop_end + ":\n";
  } else if (auto return_stmt = dynamic_cast<ReturnStmt *>(stmt)) {
    // return expr;
    if (return_stmt->value) {
      handle_expr(out, return_stmt->value.get(), var_table, data_table,
                  func_table, nullptr);
      // Result is in rax (return value)
    } else {
      out += "  xor rax, rax\n"; // Return 0
    }

    if (ctx.in_function) {
      // Return from function
      out += "  mov rsp, rbp\n";
      out += "  pop rbp\n";
      out += "  ret\n";
    } else {
      // Exit from main
      out += "  mov rdi, rax\n"; // Move return value to exit code
      out += "  mov rsp, rbp\n";
      out += "  pop rbp\n";
      out += "  mov rax, 60\n";
      out += "  syscall\n";
    }
  }
}

// Generate assembly code for a function definition
string generate_function(FunctionDef *func_def, Data_table &data_table,
                         Function_table &func_table, CodegenContext &ctx) {
  string out;

  // Generate function label
  string func_label = ctx.generate_function_label(func_def->name);
  out += func_label + ":\n";

  // Function prologue
  out += "  push rbp\n";
  out += "  mov rbp, rsp\n";

  // Count local variables in function to allocate stack space
  int local_var_count = 0;
  std::function<void(Stmt *)> count_vars = [&](Stmt *stmt) {
    if (auto var_decl = dynamic_cast<VarDeclStmt *>(stmt)) {
      local_var_count++;
    } else if (auto block = dynamic_cast<BlockStmt *>(stmt)) {
      for (const auto &s : block->statements) {
        count_vars(s.get());
      }
    } else if (auto if_stmt = dynamic_cast<IfStmt *>(stmt)) {
      count_vars(if_stmt->thenBranch.get());
      if (if_stmt->elseBranch) {
        count_vars(if_stmt->elseBranch.get());
      }
    } else if (auto while_stmt = dynamic_cast<WhileStmt *>(stmt)) {
      count_vars(while_stmt->body.get());
    }
  };
  count_vars(func_def->body.get());

  // Allocate stack space for local variables
  if (local_var_count > 0) {
    out += "  sub rsp, " + to_string(local_var_count * 8) + "\n";
  }

  // Create a new variable table for this function's scope
  Var_table local_var_table;

  // Add parameters to the variable table
  // Parameters are at [rbp + 16], [rbp + 24], etc. (after return addr and saved
  // rbp)
  for (size_t i = 0; i < func_def->parameters.size(); i++) {
    Variable param_var;
    param_var.name = func_def->parameters[i].name;
    param_var.rbp_offset =
        16 + i * 8; // Positive offset for parameters above rbp
    param_var.size = 8;
    param_var.value = nullptr;
    param_var.type = VarType::INT; // Default to INT for now
    param_var.string_label = "";
    param_var.is_param = true; // Mark as parameter
    local_var_table.table.push_back(param_var);
  }

  // Set context to indicate we're in a function
  bool prev_in_function = ctx.in_function;
  ctx.in_function = true;

  // Generate function body
  handle_stmt(out, func_def->body.get(), local_var_table, data_table,
              func_table, ctx);

  // Restore context
  ctx.in_function = prev_in_function;

  // Function epilogue (in case there's no explicit return)
  out += "  xor rax, rax\n"; // Default return value 0
  out += "  mov rsp, rbp\n";
  out += "  pop rbp\n";
  out += "  ret\n";
  out += "\n";

  return out;
}

string generate_data_header(const Data_table &data_table) {
  if (data_table.strings.empty()) {
    return "";
  }

  string data = "\nsection .data\n";
  for (const auto &str : data_table.strings) {
    // Format: label: db "string content", 0
    data += "  " + str.label + ": db \"";
    // Escape special characters in the string
    for (char c : str.value) {
      switch (c) {
      case '\n':
        data += "\", 10, \"";
      case '\t':
        data += "\", 9, \"";
      case '\"':
        data += "\\\"";
      case '\\':
        data += "\\\\";
      default:
        data += c;
      }
    }
    data += "\", 0\n";
    data += "  " + str.label + "_len equ $ - " + str.label + " - 1\n";
  }
  return data;
}

string generate_asm_program(const Program &program, Var_table &var_table,
                            Data_table &data_table, Function_table &func_table,
                            CodegenContext &ctx) {
  string out;
  string functions_code;

  // First pass: collect function definitions and generate their code
  for (const auto &stmt : program.statements) {
    if (auto func_def = dynamic_cast<FunctionDef *>(stmt.get())) {
      // Add function to function table
      vector<CodegenFunctionParam> params;
      for (const auto &param : func_def->parameters) {
        CodegenFunctionParam cgParam;
        cgParam.name = param.name;
        cgParam.type = VarType::INT; // Default to INT for now
        cgParam.isConst = param.isConst;
        params.push_back(cgParam);
      }
      string func_label = ctx.generate_function_label(func_def->name);
      func_table.add_function(func_def->name, func_label, params, VarType::INT);

      // Generate function code
      functions_code +=
          generate_function(func_def, data_table, func_table, ctx);
    }
  }

  // Count local variables in main to allocate stack space
  int local_var_count = 0;
  std::function<void(Stmt *)> count_vars = [&](Stmt *stmt) {
    if (auto var_decl = dynamic_cast<VarDeclStmt *>(stmt)) {
      local_var_count++;
    } else if (auto block = dynamic_cast<BlockStmt *>(stmt)) {
      for (const auto &s : block->statements) {
        count_vars(s.get());
      }
    } else if (auto if_stmt = dynamic_cast<IfStmt *>(stmt)) {
      count_vars(if_stmt->thenBranch.get());
      if (if_stmt->elseBranch) {
        count_vars(if_stmt->elseBranch.get());
      }
    } else if (auto while_stmt = dynamic_cast<WhileStmt *>(stmt)) {
      count_vars(while_stmt->body.get());
    }
  };

  for (const auto &stmt : program.statements) {
    if (!dynamic_cast<FunctionDef *>(stmt.get())) {
      count_vars(stmt.get());
    }
  }

  // Generate main code (_start)
  // Set up stack frame for main
  out += "  push rbp\n";
  out += "  mov rbp, rsp\n";

  // Allocate stack space for local variables
  if (local_var_count > 0) {
    out += "  sub rsp, " + to_string(local_var_count * 8) + "\n";
  }

  // Second pass: generate code for non-function statements (main code)
  for (const auto &stmt : program.statements) {
    if (!dynamic_cast<FunctionDef *>(stmt.get())) {
      handle_stmt(out, stmt.get(), var_table, data_table, func_table, ctx);
    }
  }

  // Clean up and exit (return last variable's value if any)
  if (!var_table.table.empty()) {
    const auto &last_var = var_table.table.back();
    if (last_var.is_param) {
      out += "  mov rdi, [rbp + " + std::to_string(last_var.rbp_offset) + "]\n";
    } else {
      out += "  mov rdi, [rbp - " + std::to_string(last_var.rbp_offset) + "]\n";
    }
  } else {
    out += "  xor rdi, rdi\n";
  }
  out += "  mov rsp, rbp\n";
  out += "  pop rbp\n";
  out += "  mov rax, 60\n";
  out += "  syscall\n";

  // Append function code after main
  out += "\n" + functions_code;

  return out;
}

void ast_to_bin(std::ostream &out, const Program &program) {
  Var_table var_table;
  Data_table data_table;
  Function_table func_table;
  CodegenContext ctx;

  string code =
      generate_asm_program(program, var_table, data_table, func_table, ctx);

  out << generate_asm_headers();
  out << code;
  out << generate_data_header(data_table);
}
