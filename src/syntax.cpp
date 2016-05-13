/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include <regex>
#include <cmath>
#include "parser.h"
#include "tokenizer.h"
#include "syntax.h"
#include "util.h"

/**
 * Parses a literal value in the code, such as "0x00ff" or "1234".
 * Only hex and decimal formats supported currently, no string
 * literals. Only stores up to the maximum value of a signed int.
 * Returns false if the literal is not in a valid format.
 */
bool LiteralToken::parse(const AtomToken& token) {
  _lineNum = token.line();
  if (std::regex_match(token.str(), std::regex("^0[xX][0-9a-fA-F]+$"))) {
    // Matched hex
    _value = 0;
    for (size_t i = 2; i < token.str().size(); i++) {
      char c = token.str().at(i);
      uint8_t hexVal =
        ('0' <= c && c <= '9') ? (c - '0') :
        ('a' <= c && c <= 'f') ? (c - 'a' + 10) : (c - 'A' + 10);
      _value = (_value * 16) + hexVal;
    }
  } else if (std::regex_match(token.str(), std::regex("^0[bB][01]+$"))) {
    // Matched binary
    _value = 0;
    for (size_t i = 2; i < token.str().size(); i++) {
      _value = (_value * 2) + (token.str().at(i) - '0');
    }
  } else if (std::regex_match(token.str(), std::regex("^[0-9]+$"))) {
    // Matched decimal
    _value = 0;
    for (size_t i = 0; i < token.str().size(); i++) {
      char c = token.str().at(i);
      uint8_t decVal = c - '0';
      _value = (_value * 10) + decVal;
    }
  } else {
    return false;
  }
  return true;
}

/**
 * Checks if the given token represents an operator and sets its internal
 * _op value equal to the operator. Returns false if the token does not
 * represent a valid operator.
 */
bool OperatorToken::parse(const AtomToken& token) {
  static std::vector<std::string> validOps = { "+", "-", "*", "/", "%", "=",
                                               "&", "|", "^", "~", "!", "||",
                                               "&&", "<", "<=", ">", ">=",
                                               "==", "!=", "[", "<<", ">>"  };
  _lineNum = token.line();
  for (auto op : validOps) {
    if (token.str() == op) {
      _op = op;
      return true;
    }
  }
  return false;
}

bool OperatorToken::maybeBinary() {
  static std::vector<std::string> validOps = { "+", "-", "*", "/", "%", "=",
                                               "&", "|", "^", "||", "&&", "<",
                                               "<=", ">", ">=", "==", "!=",
                                               "[", "<<", ">>" };
  for (auto op : validOps) {
    if (_op == op) {
      return true;
    }
  }
  return false;
}

bool OperatorToken::maybeUnary() {
  static std::vector<std::string> validOps = { "-", "*", "&", "~", "!", "+" };
  for (auto op : validOps) {
    if (_op == op) {
      return true;
    }
  }
  return false;
}

int OperatorToken::precedence() const {
  if ("[" == _op) {
    return 1;
  } else if (isUnary()) {
    return 2;
  } else if ("*" == _op || "/" == _op || "%" == _op) {
    return 3;
  } else if ("+" == _op || "-" == _op) {
    return 4;
  } else if ("<<" == _op || ">>" == _op) {
    return 5;
  } else if ("<" == _op || "<=" == _op || ">" == _op || ">=" == _op) {
    return 6;
  } else if ("==" == _op || "!=" == _op) {
    return 7;
  } else if ("&" == _op) {
    return 8;
  } else if ("^" == _op) {
    return 9;
  } else if ("|" == _op) {
    return 10;
  } else if ("&&" == _op) {
    return 11;
  } else if ("||" == _op) {
    return 12;
  } else if ("=" == _op) {
    return 13;
  }
  return -1;
}

bool OperatorToken::leftToRight() const {
  return isBinary();
}

uint16_t OperatorToken::operate(uint16_t lhs, uint16_t rhs) const {
  if (isUnary()) {
    if ("-" == _op) {
      return -rhs;
    } else if ("*" == _op) {
      throw "Dereferencing not allowed in constant expression.";
    } else if ("&" == _op) {
      throw "Address-of not allowed in constant expression.";
    } else if ("~" == _op) {
      return ~rhs;
    } else if ("!" == _op) {
      return !rhs ? 1 : 0;
    } else if ("+" == _op) {
      return +rhs;
    }
  } else if (isBinary()) {
    if ("+" == _op) {
      return lhs + rhs;
    } else if ("-" == _op) {
      return lhs - rhs;
    } else if ("*" == _op) {
      return lhs * rhs;
    } else if ("/" == _op) {
      if (0 == rhs) {
        _warn("Division by zero in expression.", _lineNum);
        return 0xffff;
      }
      return lhs / rhs;
    } else if ("%" == _op) {
      if (0 == rhs) {
        _warn("Division by zero in expression.", _lineNum);
        return 0xffff;
      }
      return lhs % rhs;
    } else if ("=" == _op) {
      throw "Assignment not allowed in constant expression.";
    } else if ("&" == _op) {
      return lhs & rhs;
    } else if ("|" == _op) {
      return lhs | rhs;
    } else if ("^" == _op) {
      return lhs ^ rhs;
    } else if ("||" == _op) {
      return lhs || rhs ? 1 : 0;
    } else if ("&&" == _op) {
      return lhs && rhs ? 1 : 0;
    } else if ("<" == _op) {
      return lhs < rhs ? 1 : 0;
    } else if ("<=" == _op) {
      return lhs <= rhs ? 1 : 0;
    } else if (">" == _op) {
      return lhs > rhs ? 1 : 0;
    } else if (">=" == _op) {
      return lhs >= rhs ? 1 : 0;
    } else if ("==" == _op) {
      return lhs == rhs ? 1 : 0;
    } else if ("!=" == _op) {
      return lhs != rhs ? 1 : 0;
    } else if ("[" == _op) {
      throw "Array indexing not yet supported.";
    } else if ("<<" == _op) {
      return lhs << rhs;
    } else if (">>" == _op) {
      return lhs >> rhs;
    }
  }
  throw "Invalid operator '" + _op + "' in expression.";
}

/**
 * Outputs assembly code for this operation on the given left hand and
 * right hand sides. Returns an operand representing the result, which
 * will be an address or a value depending on the operation.
 */
Operand OperatorToken::output(Parser *parser,
                              const Operand& lhs, const Operand& rhs) {
  if (isUnary()) {
    if ("-" == _op) {
      // The negative of a 2's complement number x is ~x + 1.
      operandValueToReg(parser, rhs, "M");
      parser->writeInst("MOVI N 0xffff");
      parser->writeInst("XOR M N");
      parser->writeInst("MOVI N 0x1");
      parser->writeInst("ADD M N");
      parser->writeInst("PUSH M");
      return Operand(OperandType::VALUE);
    } else if ("*" == _op) {
      // Dereference operator. Do nothing for value operands, because
      // we would just be popping them off and pushing them back onto
      // the stack.
      if (OperandType::VALUE != rhs.type()) {
        operandValueToReg(parser, rhs, "M");
        parser->writeInst("PUSH M");
      }
      return Operand(OperandType::ADDRESS);
    } else if ("&" == _op) {
      // Do nothing, the value should already be on the stack.
      if (OperandType::ADDRESS != rhs.type()) {
        throw "Right hand side must be an address for the address-of operator.";
      }
      return Operand(OperandType::VALUE);
    } else if ("~" == _op) {
      // x ^ 0xffff == ~x
      operandValueToReg(parser, rhs, "M");
      parser->writeInst("MOVI N 0xffff");
      parser->writeInst("XOR M N");
      parser->writeInst("PUSH M");
      return Operand(OperandType::VALUE);
    } else if ("!" == _op) {
      // x = x != 0 ? 1 : 0
      std::string label1 = parser->getUnusedLabel("label");
      std::string label2 = parser->getUnusedLabel("label");
      operandValueToReg(parser, rhs, "M");
      parser->writeInst("TST M M");
      parser->writeInst("JNE " + label1);
      parser->writeInst("MOVI M 0x1");
      parser->writeInst("JMPI " + label2);
      parser->writeln(label1 + ":");
      parser->writeInst("MOVI M 0x0");
      parser->writeln(label2 + ":");
      parser->writeInst("PUSH M");
      return Operand(OperandType::VALUE);
    } else if ("+" == _op) {
      // Get the value and push it onto the stack.
      operandValueToReg(parser, rhs, "M");
      parser->writeInst("PUSH M");
      return Operand(OperandType::VALUE);
    }
  } else if (isBinary()) {
    if ("%" == _op) {
      // a % b == a - (b * (a / b))
      operandValueToReg(parser, rhs, "N");
      operandValueToReg(parser, lhs, "M");
      parser->writeInst("MOV L M");
      parser->writeInst("DIV M N");
      parser->writeInst("MUL M N");
      parser->writeInst("SUB L M");
      parser->writeInst("PUSH L");
      return Operand(OperandType::VALUE);
    } else if ("=" == _op) {
      // Load RHS value into a register.
      operandValueToReg(parser, rhs, "N");
      if (OperandType::ADDRESS == lhs.type()) {
        parser->writeInst("POP M");
        parser->writeInst("STOR N M");
      } else if (OperandType::REGISTER == lhs.type()) {
        parser->writeInst("MOV " + lhs.reg() + " N");
      } else {
        throw "Left hand side of assignment cannot be an rvalue.";
      }
      parser->writeInst("PUSH N");
      return Operand(OperandType::VALUE);
    } else if ("+" == _op || "-" == _op || "*" == _op || "/" == _op ||
               "&" == _op || "|" == _op || "^" == _op ||
               "<<" == _op || ">>" == _op) {
      operandValueToReg(parser, rhs, "N");
      operandValueToReg(parser, lhs, "M");
      std::string inst;
      if ("+" == _op) {
        inst = "ADD";
      } else if ("-" == _op) {
        inst = "SUB";
      } else if ("*" == _op) {
        inst = "MUL";
      } else if ("/" == _op) {
        inst = "DIV";
      } else if ("&" == _op) {
        inst = "AND";
      } else if ("|" == _op) {
        inst = "OR";
      } else if ("^" == _op) {
        inst = "XOR";
      } else if ("<<" == _op) {
        inst = "SHL";
      } else if (">>" == _op) {
        inst = "SHRL";
      }
      parser->writeInst(inst + " M N");
      parser->writeInst("PUSH M");
      return Operand(OperandType::VALUE);
    } else if ("[" == _op) {
      // For x[a], push &x + (a * DATA_SIZE) onto the stack.
      operandValueToReg(parser, rhs, "N");
      operandValueToReg(parser, lhs, "M");
      // Assume DATA_SIZE is a power of 2, and do a fast
      // multiply by shifting left
      parser->writeInst("MOVI L " + toHexStr(ceil(log2(DATA_SIZE))));
      parser->writeInst("SHL N L");
      parser->writeInst("ADD M N");
      parser->writeInst("PUSH M");
      return Operand(OperandType::ADDRESS);
    } else if ("||" == _op || "&&" == _op) {
      // Make N either 0 or 1.
      std::string label1 = parser->getUnusedLabel("label");
      std::string label2 = parser->getUnusedLabel("label");
      operandValueToReg(parser, rhs, "N");
      parser->writeInst("TST N N");
      parser->writeInst("JEQ " + label1);
      parser->writeInst("MOVI N 0x1");
      parser->writeInst("JMPI " + label2);
      parser->writeln(label1 + ":");
      parser->writeInst("MOVI N 0x0");
      parser->writeln(label2 + ":");
      // Make M either 0 or 1.
      std::string label3 = parser->getUnusedLabel("label");
      std::string label4 = parser->getUnusedLabel("label");
      operandValueToReg(parser, lhs, "M");
      parser->writeInst("TST M M");
      parser->writeInst("JEQ " + label3);
      parser->writeInst("MOVI M 0x1");
      parser->writeInst("JMPI " + label4);
      parser->writeln(label3 + ":");
      parser->writeInst("MOVI M 0x0");
      parser->writeln(label4 + ":");
      // Do the operation.
      if ("||" == _op) {
        parser->writeInst("OR M N");
      } else if ("&&" == _op) {
        parser->writeInst("AND M N");
      }
      // Push the result.
      parser->writeInst("PUSH M");
      return Operand(OperandType::VALUE);
    } else if ("<" == _op || "<=" == _op || ">" == _op || ">=" == _op ||
               "==" == _op || "!=" == _op) {
      std::string label1 = parser->getUnusedLabel("label");
      std::string label2 = parser->getUnusedLabel("label");
      operandValueToReg(parser, rhs, "N");
      operandValueToReg(parser, lhs, "M");
      parser->writeInst("CMP M N");
      std::string inst;
      if ("<" == _op) {
        inst = "JB";
      } else if ("<=" == _op) {
        inst = "JBE";
      } else if (">" == _op) {
        inst = "JA";
      } else if (">=" == _op) {
        inst = "JAE";
      } else if ("==" == _op) {
        inst = "JEQ";
      } else if ("!=" == _op) {
        inst = "JNE";
      }
      parser->writeInst(inst + " " + label1);
      parser->writeInst("MOVI M 0x0");
      parser->writeInst("JMPI " + label2);
      parser->writeln(label1 + ":");
      parser->writeInst("MOVI M 0x1");
      parser->writeln(label2 + ":");
      parser->writeInst("PUSH M");
      return Operand(OperandType::VALUE);
    }
  }
  // By default return a value type, control should never reach here.
  return Operand(OperandType::VALUE);
}

/**
 * Parses either a single type or an array type, like "uint16"
 * or "uint16[32]". The expression within square brackets must
 * be known at compile time.
 */
bool TypeToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars) {
  AtomToken typeName = tokenizer->getNext();
  if (!isType(typeName.str())) {
    _error("Invalid type '" + typeName.str() + "'.", typeName.line());
    return false;
  }
  _lineNum = typeName.line();
  _name = typeName.str();
  if ("[" == tokenizer->peekNext().str()) {
    tokenizer->getNext();
    _isArray = true;
    ExprToken expr;
    if (!expr.parse(tokenizer, functions, globals, parameters, localVars)) {
      return false;
    }
    if (!expr.isConst()) {
      _error("Array size must be known at compile time.", expr.line());
      return false;
    }
    _arraySize = expr.val();
    if (!_expect(tokenizer, "]")) {
      return false;
    }
  }
  return true;
}

/**
 * Constructs an expression token that represents a constant value.
 */
ExprToken::ExprToken(uint16_t value) : _const(true), _value(value) {
  _postfix.push_back(std::shared_ptr<Token>(new LiteralToken(value)));
}

/**
 * Parses an expression from infix to postfix notation, then validates it,
 * then evaluates the expression if it can be known at compile time. Returns
 * false if there is anything wrong with the expression. Sets the _const and
 * _value internal variables appropriately.
 */
bool ExprToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars) {
  std::string prev;
  std::stack<std::string> parens;
  std::stack<std::shared_ptr<Token>> opStack;
  while (true) {
    AtomToken t = tokenizer->peekNext();
    if (-1 == _lineNum) {
      _lineNum = t.line();
    }
    std::shared_ptr<LiteralToken> literal(new LiteralToken());
    std::shared_ptr<OperatorToken> op(new OperatorToken());
    if ("(" == t.str()) {
      if (!prev.empty() && "(" != prev && "op" != prev) {
        _error("Unexpected token '" + t.str() + "' in expression.", t.line());
        return false;
      }
      prev = t.str();
      parens.push(t.str());
      opStack.push(std::shared_ptr<Token>(new AtomToken("(", t.line())));
    } else if (")" == t.str() || "]" == t.str()) {
      if ((!parens.empty() && otherParen(t.str()) != parens.top()) ||
          (")" != prev && "val" != prev)) {
        _error("Unexpected token '" + t.str() + "' in expression.", t.line());
        return false;
      } else if (parens.empty()) {
        break;
      }
      // Pop operators off the stack until we reach a corresponding "("
      while (!opStack.empty() &&
             std::dynamic_pointer_cast<OperatorToken>(opStack.top())) {
        _postfix.push_back(opStack.top());
        opStack.pop();
      }
      // Pop off the "("
      opStack.pop();
      parens.pop();
      prev = ")";
    } else if (literal->parse(t)) {
      if (!prev.empty() && "(" != prev && "op" != prev) {
        _error("Unexpected token '" + t.str() + "' in expression.", t.line());
        return false;
      }
      prev = "val";
      _postfix.push_back(literal);
    } else if (op->parse(t)) {
      // Determine if the operator is binary or unary.
      if (op->maybeBinary() && (")" == prev || "val" == prev)) {
        op->setBinary();
      } else if (op->maybeUnary() &&
                 (prev.empty() || "(" == prev || "op" == prev)) {
        op->setUnary();
      } else {
        _error("Unexpected token '" + t.str() + "' in expression.", t.line());
        return false;
      }
      // Handle the operator stack based on precedence.
      std::shared_ptr<OperatorToken> topOp;
      while (!opStack.empty() &&
             (topOp = std::dynamic_pointer_cast<OperatorToken>(opStack.top()))) {
        if (op->precedence() < topOp->precedence()) {
          break;
        } else if (op->precedence() == topOp->precedence()) {
          if (op->leftToRight()) {
            _postfix.push_back(opStack.top());
            opStack.pop();
          }
          break;
        } else {
          _postfix.push_back(opStack.top());
          opStack.pop();
        }
      }
      opStack.push(op);
      // If it's an open square bracket, add it to the parentheses stack.
      // Also push an open parenthesis to the operator stack, since the
      // expression inside [] is treated as if it were parenthesized.
      if ("[" == t.str()) {
        parens.push(t.str());
        opStack.push(std::shared_ptr<Token>(new AtomToken("(", op->line())));
      }
      prev = "op";
    } else if (isValidName(t.str())) {
      auto globalVar = getGlobal(t.str(), globals);
      auto function = getFunction(t.str(), functions);
      auto param = getParameter(t.str(), parameters);
      auto localVar = getLocal(t.str(), localVars);
      if (nullptr != globalVar) {
        // If the name represents a global variable, push it onto the stack
        _postfix.push_back(globalVar);
        prev = "val";
      } else if (nullptr != param) {
        // If the name represents a parameter, push it onto the stack
        _postfix.push_back(param);
        prev = "val";
      } else if (nullptr != localVar) {
        // If the name represents a parameter, push it onto the stack
        _postfix.push_back(localVar);
        prev = "val";
      } else if (nullptr != function) {
        // If the function returns void, this is an error. We can't have
        // void functions mixed in with expressions.
        if ("void" == function->type().name()) {
          _error("Function call to 'void " + function->name() + "()' not "
                 "allowed in expression.",
                 t.line());
          return false;
        }
        // If it is not void, parse the function call.
        std::shared_ptr<FunctionCallToken> fnCall(new FunctionCallToken());
        if (!fnCall->parse(tokenizer, functions, globals,
                           parameters, localVars)) {
          return false;
        }
        _postfix.push_back(fnCall);
        prev = "val";
        // Continue so we don't consume an extra token at the end, all
        // tokens have been consumed already for the function call.
        continue;
      } else {
        _error("Unknown token '" + t.str() + "'.", t.line());
        return false;
      }
    } else if (t.str().empty()) {
      if (!parens.empty() || (")" != prev && "val" != prev)) {
        _error("Unexpected EOF in expression.", t.line());
        return false;
      }
      break;
    } else {
      if (!parens.empty() || (")" != prev && "val" != prev)) {
        _error("Unexpected token '" + t.str() + "' in expression.", t.line());
        return false;
      }
      break;
    }
    // We actually used the token we peeked at, so consume it here.
    tokenizer->getNext();
  }
  // Pop any remaining operators off the stack.
  while (!opStack.empty()) {
    _postfix.push_back(opStack.top());
    opStack.pop();
  }
  // Validate the expression for further errors.
  if (!_validate()) {
    return false;
  }
  // Evaluate the expression, if it is known at compile-time.
  _evaluate();
  // Flag variables that are not able to be stored in registers due to
  // address-of operations.
  _flagNonRegs();
  return true;
}

/**
 * Validate the assignments in the expression, make sure that we only
 * assign to types that can be assigned to.
 */
bool ExprToken::_validate() {
  std::stack<std::string> operands;
  for (auto token : _postfix) {
    auto op = std::dynamic_pointer_cast<OperatorToken>(token);
    if (std::dynamic_pointer_cast<LiteralToken>(token) ||
        std::dynamic_pointer_cast<FunctionCallToken>(token)) {
      operands.push("rvalue");
    } else if (std::dynamic_pointer_cast<GlobalVarToken>(token) ||
               std::dynamic_pointer_cast<ParamToken>(token) ||
               std::dynamic_pointer_cast<LocalVarToken>(token)) {
      operands.push("lvalue");
    } else if (nullptr != op) {
      std::string rhs = operands.top();
      operands.pop();
      std::string lhs;
      if (op->isBinary()) {
        lhs = operands.top();
        operands.pop();
      }
      std::string result;
      if ("=" == op->str()) {
        if ("rvalue" == lhs) {
          _error("Can't assign to an rvalue in expression.", op->line());
          return false;
        }
        result = "rvalue";
      } else if ("*" == op->str() && op->isUnary()) {
        result = "lvalue";
      } else if ("&" == op->str() && op->isUnary()) {
        if ("lvalue" != rhs) {
          _error("Can't get address of an rvalue in expression.",
                 op->line());
          return false;
        }
        result = "rvalue";
      } else if ("[" == op->str()) {
        result = "lvalue";
      } else {
        result = "rvalue";
      }
      operands.push(result);
    }
  }
  return true;
}

/**
 * Tries to evaluate the expression as if it were constant, setting
 * _const appropriately and _value if succesful. Warns of certain errors
 * like divide by zero.
 */
void ExprToken::_evaluate() {
  // Evaluate the postfix expression
  std::stack<std::shared_ptr<Token>> operands;
  for (auto token : _postfix) {
    auto op = std::dynamic_pointer_cast<OperatorToken>(token);
    if (std::dynamic_pointer_cast<LiteralToken>(token) ||
        std::dynamic_pointer_cast<GlobalVarToken>(token)) {
      operands.push(token);
    } else if (std::dynamic_pointer_cast<ParamToken>(token) ||
               std::dynamic_pointer_cast<LocalVarToken>(token) ||
               std::dynamic_pointer_cast<FunctionCallToken>(token)) {
      // We don't know parameter or local variable values at
      // compile time, nor do we know the output of functions,
      // so this expression can't be constant.
      _const = false;
      return;
    } else if (nullptr != op) {
      std::shared_ptr<Token> rhs = operands.top();
      operands.pop();
      std::shared_ptr<Token> lhs;
      if (op->isBinary()) {
        lhs = operands.top();
        operands.pop();
      } else {
        lhs = nullptr;
      }
      uint16_t result;
      // If using assignment, dereferencing, or address-of, this
      // expression is not considered constant.
      if (("=" == op->str() && op->isBinary()) ||
          (("&" == op->str() || "*" == op->str()) && op->isUnary())) {
        _const = false;
        return;
      } else if ("[" == op->str() && op->isBinary()) {
        auto global = std::dynamic_pointer_cast<GlobalVarToken>(lhs);
        if (nullptr == global) {
          _const = false;
          return;
        }
        if (!global->isArray()) {
          _const = false;
          return;
        } else if (global->arraySize() <= rhs->val()) {
          _warn("Array index out of bounds in expression.", _lineNum);
          _const = false;
          return;
        }
        result = global->arrayVal(rhs->val());
      } else {
        result = op->operate(lhs ? lhs->val() : 0, rhs->val());
      }
      operands.push(std::shared_ptr<Token>(new LiteralToken(result)));
    } else {
      _const = false;
      return;
    }
  }
  _value = operands.top()->val();
}

/**
 * Flags variables used in this expression that are not able to be stored
 * in registers due to address-of operations.
 */
void ExprToken::_flagNonRegs() {
  std::stack<std::shared_ptr<Token>> operands;
  for (auto token : _postfix) {
    auto op = std::dynamic_pointer_cast<OperatorToken>(token);
    if (nullptr != op) {
      std::shared_ptr<Token> rhs = operands.top();
      operands.pop();
      if (op->isBinary()) {
        operands.pop();
      }
      // Check if it is an address-of operation
      if (op->isUnary() && "&" == op->str()) {
        auto var = std::dynamic_pointer_cast<Variable>(rhs);
        if (nullptr != var) {
          var->flagNonReg();
        }
      }
      // Push something back onto the stack.
      operands.push(nullptr);
    } else {
      operands.push(token);
    }
  }
}

/**
 * Outputs assembly code to evaluate the given expression and store the
 * result in the given register, reg.
 */
void ExprToken::output(Parser *parser, const VarLocation& varLoc) {
  // Evaluate the postfix expression using the stack, pushing nullptr
  // as an operand where a temporary value would go (one that is
  // not already represented by a Token).
  std::stack<Operand> operands;
  for (auto token : _postfix) {
    auto op = std::dynamic_pointer_cast<OperatorToken>(token);
    auto global = std::dynamic_pointer_cast<GlobalVarToken>(token);
    auto var = std::dynamic_pointer_cast<Variable>(token);
    auto literal = std::dynamic_pointer_cast<LiteralToken>(token);
    auto fnCall = std::dynamic_pointer_cast<FunctionCallToken>(token);
    if (nullptr != op) {
      // Pop one or two operands off the stack, depending on if the
      // operator is unary or binary. Then output the operation in
      // assembly.
      Operand rhs = operands.top();
      operands.pop();
      Operand lhs;
      if (op->isBinary()) {
        lhs = operands.top();
        operands.pop();
      }
      Operand result = op->output(parser, lhs, rhs);
      operands.push(result);
    } else if (nullptr != global) {
      // Push the address onto the stack.
      parser->writeInst("MOVI L " + global->name());
      parser->writeInst("PUSH L");
      operands.push(Operand(OperandType::ADDRESS));
    } else if (nullptr != var) {
      // Push nothing onto the stack for a register, or the address onto the
      // stack for a variable on the stack.
      if (var->isReg()) {
        operands.push(Operand(OperandType::REGISTER, var->getReg()));
      } else {
        // Get the variable's location into register M
        parser->writeInst("MOV M FP");
        int offset = var->getOffset();
        if (0 != offset) {
          parser->writeInst("MOVI L " + toHexStr(0 < offset ? offset : -offset));
          if (0 < offset) {
            parser->writeInst("ADD M L");
          } else {
            parser->writeInst("SUB M L");
          }
        }
        // Push the address onto the stack.
        parser->writeInst("PUSH M");
        operands.push(Operand(OperandType::ADDRESS));
      }
    } else if (nullptr != literal) {
      // Don't do anything with the stack, we can save this literal for
      // later use.
      operands.push(Operand(OperandType::LITERAL, literal->val()));
    } else if (nullptr != fnCall) {
      // Get the result of the function call and push it onto the stack.
      fnCall->output(parser);
      parser->writeInst("PUSH L");
      // The result of a function call is a value token.
      operands.push(Operand(OperandType::VALUE));
    }
  }
  // Move the evaluated expression output to the given variable location.
  if (varLoc.isReg()) {
    operandValueToReg(parser, operands.top(), varLoc.getReg());
  } else {
    // Get the variable's address in a register so that we can store to
    // that address.
    int offset = varLoc.getOffset();
    parser->writeInst("MOV M FP");
    if (0 != offset) {
      if (offset < 0) {
        parser->writeInst("MOVI L " + toHexStr(-offset));
        parser->writeInst("SUB M L");
      } else {
        parser->writeInst("MOVI L " + toHexStr(offset));
        parser->writeInst("ADD M L");
      }
    }
    // Pop the result from the expression and store it in the calculated
    // address.
    operandValueToReg(parser, operands.top(), "L");
    parser->writeInst("STOR L M");
  }
}

/**
 * Parses an array expression like "{1,2,3}" where 1, 2, and 3 could
 * be an arbitrary non-array expression.
 */
bool ArrayExprToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars) {
  _lineNum = tokenizer->peekNext().line();
  // Make sure the first token is a '{' symbol
  if (!_expect(tokenizer, "{")) {
    return false;
  }
  // Check if the next token is a closing brace, in which
  // case we don't need to check for expressions.
  if ("}" == tokenizer->peekNext().str()) {
    tokenizer->getNext();
    return true;
  }
  // Get any expressions we find, separated by commas
  while (true) {
    std::shared_ptr<ExprToken> expr(new ExprToken());
    if (!expr->parse(tokenizer, functions, globals, parameters, localVars)) {
      return false;
    }
    _exprs.push_back(expr);
    AtomToken next = tokenizer->getNext();
    if ("}" == next.str()) {
      break;
    } else if (next.empty()) {
      _error("Unexpected EOF.", next.line());
      return false;
    } else if ("," != next.str()) {
      _error("Unexpected token '" + next.str() + "'.", next.line());
      return false;
    }
  }
  return true;
}

/**
 * Parses a function call within an expression. A function call is of the
 * form "FUNC_NAME ( [ARG_LIST] )", where ARG_LIST is an optional
 * comma-separated list of expressions.
 */
bool FunctionCallToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars) {
  _lineNum = tokenizer->peekNext().line();
  // Start by getting the name of the function we are calling.
  AtomToken nameToken = tokenizer->getNext();
  if (nameToken.str().empty()) {
    _error("Unexpected EOF.", nameToken.line());
    return false;
  } else if (!isValidName(nameToken.str())) {
    _error("Invalid name for a function.", nameToken.line());
    return false;
  }
  _funcName = nameToken.str();
  // Next get the pointer to the function so that we know it exists
  // and can validate the arguments with the parameters later on.
  auto function = getFunction(_funcName, functions);
  if (!function) {
    _error("Function '" + _funcName + "' does not exist.", _lineNum);
    return false;
  }
  // The next token should be an open parenthesis.
  if (!_expect(tokenizer, "(")) {
    return false;
  }
  // Now get the comma-separated list of expressions.
  if (")" != tokenizer->peekNext().str()) {
    while (true) {
      std::shared_ptr<ExprToken> expr(new ExprToken());
      if (!expr->parse(tokenizer, functions, globals, parameters, localVars)) {
        return false;
      }
      _arguments.push_back(expr);
      // If the next token is not a comma, we are done. Otherwise consume
      // the comma and continue.
      if ("," != tokenizer->peekNext().str()) {
        break;
      }
      tokenizer->getNext();
    }
  }
  // Consume the closing parenthesis.
  if (!_expect(tokenizer, ")")) {
    return false;
  }
  // Check that the number of parameters is correct.
  if (this->numArgs() != function->numParams()) {
    _error("Invalid function call, expected " +
           std::to_string(function->numParams()) + " arguments but got " +
           std::to_string(this->numArgs()) + ".",
           _lineNum);
    return false;
  }
  // Make sure this is not a call to "void main()", which is illegal.
  if ("main" == function->name() && "void" == function->type().name() &&
      0 == function->numParams()) {
    _error("Illegal call to 'void main()', the entry point cannot be called "
	   "from within the program.",
	   _lineNum);
    return false;
  }
  return true;
}

/**
 * Outputs the assembly code for this function call.
 */
void FunctionCallToken::output(Parser *parser) {
  // Check if the function is a builtin, in which case the assembly
  // output will look different (with no CALL instruction).
  if ("COLOR" == _funcName) {
    // Signature is "void COLOR(uint16 color)"
    _arguments[0]->output(parser, VarLocation("M"));
    parser->writeInst("COLOR M");
  } else if ("PIXEL" == _funcName) {
    // Signature is "void PIXEL(uint16 x, uint16 y)"
    _arguments[0]->output(parser, VarLocation("M"));
    _arguments[1]->output(parser, VarLocation("N"));
    parser->writeInst("PIXEL M N");
  } else if ("TIMERST" == _funcName) {
    // Signatue is "void TIMERST()"
    parser->writeInst("TIMERST");
  } else if ("TIME" == _funcName) {
    // Signature is "uint16 TIME()"
    parser->writeInst("TIME L");
  } else if ("INPUT" == _funcName) {
    // Signature is "uint16 INPUT(uint16 input_id)"
    _arguments[0]->output(parser, VarLocation("M"));
    parser->writeInst("INPUT L M");
  } else if ("RND" == _funcName) {
    // Signature is "uint16 RND()"
    parser->writeInst("RND L");
  } else {
    // Save registers A through D if we are using them as arguments.
    std::stack<std::string> savedRegisters;
    std::string reg = "A";
    for (size_t i = 0; i < 4 && i < _arguments.size(); i++, reg[0]++) {
      parser->writeInst("PUSH " + reg);
      savedRegisters.push(reg);
    }
    // Evaluate up to the first four arguments and store them in registers
    // A through D.
    reg = "A";
    for (size_t i = 0; i < 4 && i < _arguments.size(); i++, reg[0]++) {
      _arguments[i]->output(parser, VarLocation(reg));
    }
    // Push any overflow arguments onto the stack. In reverse order so that
    // it matches the callee's expectations of the order.
    for (int i = _arguments.size() - 1; 4 <= i; i--) {
      _arguments[i]->output(parser, VarLocation("L"));
      parser->writeInst("PUSH L");
    }
    // Call the function
    parser->writeInst("CALL " + _funcName);
    // Restore registers A through D if they were used as arguments.
    while (!savedRegisters.empty()) {
      parser->writeInst("POP " + savedRegisters.top());
      savedRegisters.pop();
    }
  }
}

/**
 * Parses a global variable declaration and possibly assignment.
 * Assigns a value to the global variable, either zero if not
 * assigned or the value of the constant expression it is assigned to.
 * Returns false if the expression it is assigned to cannot be known
 * at compile time, or if there is another syntax error.
 */
bool GlobalVarToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals) {
  _lineNum = _type.line();
  // Validate the type
  if ("void" == _type.name()) {
    _error("Global var cannot be of type 'void'.", _type.line());
    return false;
  }
  // Validate the name
  if (!isValidName(_name)) {
    _error("Invalid global var name '" + _name + "'.", _type.line());
    return false;
  } else if (getFunction(_name, functions)) {
    _error("Global var '" + _name + "' conflicts with existing function name.",
           _type.line());
    return false;
  } else if (getGlobal(_name, globals)) {
    _error("Global var '" + _name + "' conflicts with existing global var name.",
           _type.line());
    return false;
  }
  // Validate the value (if set)
  AtomToken next = tokenizer->getNext();
  AtomToken last = next;
  if ("=" == next.str()) {
    if (_type.isArray()) {
      // Make sure array expression has as many values as the type
      // requires, and make sure they are all constant.
      ArrayExprToken arrayExpr;
      if (!arrayExpr.parse(tokenizer, functions, globals)) {
        return false;
      } else if (arrayExpr.size() != _type.arraySize()) {
        _error("Array size mismatch.", _type.line());
        return false;
      }
      for (size_t i = 0; i < arrayExpr.size(); i++) {
        auto expr = arrayExpr.get(i);
        if (!expr->isConst()) {
          _error("Global value must be known at compile time.", expr->line());
          return false;
        }
        _arrayValues.push_back(expr->val());
      }
    } else {
      // Not an array value, get the singleton initialization expression
      ExprToken expr;
      if (!expr.parse(tokenizer, functions, globals)) {
        return false;
      } else if (!expr.isConst()) {
        _error("Global value must be known at compile time.", expr.line());
        return false;
      }
      _value = expr.val();
    }
    // This should be a semicolon
    last = tokenizer->getNext();
  } else if (";" == next.str()) {
    // No value supplied, give default value of 0.
    if (_type.isArray()) {
      for (size_t i = 0; i < _type.arraySize(); i++) {
        _arrayValues.push_back(0);
      }
    } else {
      _value = 0;
    }
  }
  if (last.empty()) {
    _error("Unexpected EOF.", last.line());
    return false;
  } else if (";" != last.str()) {
    _error("Unexpected token '" + last.str() + "', expected ';'.", last.line());
    return false;
  }
  return true;
}

/**
 * Output assembly code for this global variable declaration.
 */
void GlobalVarToken::output(Parser *parser) {
  // Write out a label for the global variable.
  parser->writeln(_name + ":");
  if (_type.isArray()) {
    // The address for the array's elements will be the current byte
    // position plus the instruction size.
    parser->writeData(toHexStr(parser->getBytePos() + INST_SIZE), 1);
    // Write out the array's elements as hex values.
    std::string dataOutput;
    for (size_t i = 0; i < _arrayValues.size(); i++) {
      dataOutput += toHexStr(_arrayValues[i]);
      if (i + 1 < _arrayValues.size()) {
        dataOutput += " ";
      }
    }
    parser->writeData(dataOutput, _arrayValues.size());
  } else {
    // Not an array, just write out the single hex value.
    parser->writeData(toHexStr(_value), 1);
  }
}

/**
 * Parses out a type and a name for the parameter, making sure
 * the type is appropriate for a parameter and the name doesn't
 * conflict with a function or global variable name.
 *
 * TODO: Implement default values.
 */
bool ParamToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals) {
  if (!_type.parse(tokenizer, functions, globals)) {
    return false;
  } else if (_type.isArray()) {
    _error("Array parameter types not supported.", _type.line());
    return false;
  } else if ("void" == _type.name()) {
    _error("Parameter cannot be of type void.", _type.line());
    return false;
  }
  AtomToken nameToken = tokenizer->getNext();
  if (nameToken.str().empty()) {
    _error("Unexpected EOF.", nameToken.line());
    return false;
  } else if (!isValidName(nameToken.str())) {
    _error("Invalid parameter name '" + nameToken.str() + "'.",
           nameToken.line());
    return false;
  } else if (getFunction(nameToken.str(), functions)) {
    _error("Parameter name '" + nameToken.str() + "' conflicts with "
           "function name.", nameToken.line());
    return false;
  } else if (getGlobal(nameToken.str(), globals)) {
    _error("Parameter name '" + nameToken.str() + "' conflicts with "
           " global var name.", nameToken.line());
    return false;
  }
  _name = nameToken.str();
  return true;
}

/**
 * Parses a function's parameter signature and the function body.
 *
 * TODO: Allow function definitions.
 */
bool FunctionToken::parse(
      Tokenizer *tokenizer,
      std::vector<std::shared_ptr<FunctionToken>>& functions,
      std::vector<std::shared_ptr<GlobalVarToken>>& globals) {
  _lineNum = _type.line();
  // Validate the type
  if (_type.isArray()) {
    _error("Function return type cannot be array-valued.", _type.line());
    return false;
  }
  // Validate the name
  if (!isValidName(_name)) {
    _error("Invalid function name '" + _name + "'.", _type.line());
    return false;
  } else if (getFunction(_name, functions)) {
    _error("Function '" + _name + "' conflicts with existing function name.",
           _type.line());
    return false;
  } else if (getGlobal(_name, globals)) {
    _error("Function '" + _name + "' conflicts with existing global var name.",
           _type.line());
    return false;
  }
  // Make sure the first token is an open parenthesis
  if (!_expect(tokenizer, "(")) {
    return false;
  }
  // Get the parameters
  while (")" != tokenizer->peekNext().str()) {
    std::shared_ptr<ParamToken> param(new ParamToken());
    if (!param->parse(tokenizer, functions, globals)) {
      return false;
    } else if (getParameter(param->name(), _parameters)) {
      _error("Parameter '" + param->name() + "' conflicts with existing "
             "parameter name.", param->line());
      return false;
    }
    _parameters.push_back(param);
    AtomToken t = tokenizer->peekNext();
    if (t.str().empty()) {
      _error("Unexpected EOF.", t.line());
      return false;
    } else if ("," == t.str()) {
      tokenizer->getNext();
      continue;
    } else if (")" != t.str()) {
      _error("Unexpected token '" + t.str() + "'.", t.line());
      return false;
    }
  }
  // Consume the closing parenthesis
  tokenizer->getNext();
  // Add self to the functions list
  functions.push_back(shared_from_this());
  // Get the function body. Make sure it starts with a '{'.
  if (!_expect(tokenizer, "{")) {
    return false;
  }
  // Get the statements within the function body. Local variable
  // declarations must come before any other statements.
  bool inDeclarations = true;
  while ("}" != tokenizer->peekNext().str()) {
    auto statement = StatementToken::parse(tokenizer, functions, globals,
                                           _parameters, _localVars, _labels,
                                           _gotos, shared_from_this());
    if (!statement) {
      return false;
    }
    // If it's a local variable declaration, add it to the list of local
    // variables.
    if (std::dynamic_pointer_cast<LocalVarToken>(statement)) {
      if (!inDeclarations) {
        _error("Declarations must come before other "
               "statements in function '" + _name + "()'.",
               statement->line());
        return false;
      }
      _localVars.push_back(std::dynamic_pointer_cast<LocalVarToken>(statement));
    } else {
      inDeclarations = false;
      _statements.push_back(statement);
    }
    // Check for EOF
    AtomToken t = tokenizer->peekNext();
    if (t.str().empty()) {
      _error("Unexpected EOF.", t.line());
      return false;
    }
  }
  // Consume the '}' token
  tokenizer->getNext();
  // Make sure all of the goto statements match up with a label.
  bool ret = true;
  for (auto gotoStatement : _gotos) {
    if (!getLabel(gotoStatement->label(), _labels)) {
      _error("Label '" + gotoStatement->label() + "' does not exist in "
             "function '" + _name + "' for goto statement.",
             gotoStatement->line());
      ret = false;
    }
  }
  return ret;
}

/**
 * Outputs assembly code for this function.
 */
void FunctionToken::output(Parser *parser) {
  // Check if this is a builtin function, in which case we should
  // do nothing.
  if (isBuiltin(_name)) {
    return;
  }
  // Create an end label for the function, so if we return we can jump
  // to it without having to unwind the stack each time.
  std::string endLabel = parser->getUnusedLabel(_name + "_end");
  // Create a label for the function so that we can CALL it.
  parser->writeln(_name + ":");
  // Assign registers or stack positions to parameters. Parameters can
  // be stored in registers "A" through "D", and if there are more than
  // 4 parameters they will be stored on the stack before the return
  // address. The return address is stored at FP, so the first
  // overflow parameter will be stored at -2, the next at -4, etc.
  std::string reg = "A";
  int offset = -ADDRESS_SIZE;
  int numOverflowParams = 0;
  for (auto param : _parameters) {
    if (reg[0] <= 'D') {
      param->setReg(reg);
      reg[0]++;
    } else {
      param->setOffset(offset);
      offset -= DATA_SIZE;
      numOverflowParams++;
    }
  }
  // Assign registers or stack positions to local variables. Local
  // variables can be stored in registers "E" through "K", and if there
  // are more local variables than can fit in registers we store them
  // as frame pointer offsets. The offset starts at 0 and increases from
  // there.
  reg = "E";
  offset = 0;
  int extraParamOffset = 0;
  for (auto local : _localVars) {
    if (reg[0] <= 'K' && local->canBeReg()) {
      local->setReg(reg);
      // This is a callee-saved register, push it onto the stack and
      // make a note that we need to pop it later.
      parser->writeInst("PUSH " + reg);
      _savedRegisters.push(reg);
      extraParamOffset -= DATA_SIZE;
      // Increment the register
      reg[0]++;
    } else {
      local->setOffset(offset);
      offset += DATA_SIZE;
    }
    // If this is an array, reserve space for the array's data.
    if (local->type().isArray()) {
      // The data offset is at the current stack offset + DATA_SIZE.
      // We must add DATA_SIZE because the stack pointer points at an
      // address that's in use, and we want the next unused address.
      local->setDataOffset(offset + DATA_SIZE);
      offset += local->type().arraySize() * DATA_SIZE;
    }
  }
  // Save the previous value of the frame pointer.
  parser->writeInst("PUSH FP");
  _savedRegisters.push("FP");
  extraParamOffset -= DATA_SIZE;
  // Set the frame pointer to the stack's current location.
  parser->writeInst("MOV FP SP");
  // Store parameters on the stack if they are currently stored in
  // registers but are flagged as needing their own address. Also fix
  // offset for parameters on the stack based on the number of saved
  // registers.
  for (auto param : _parameters) {
    if (param->isReg() && !param->canBeReg()) {
      parser->writeInst("PUSH " + param->getReg());
      param->setOffset(offset);
      offset += DATA_SIZE;
    } else if (!param->isReg()) {
      param->setOffset(param->getOffset() + extraParamOffset);
    }
  }
  // Reserve space for the local variable storage on the stack.
  if (0 < offset) {
    parser->writeInst("MOVI L " + toHexStr(offset));
    parser->writeInst("ADD SP L");
  }

  // Outputs initial values of local variables.
  for (auto local : _localVars) {
    local->output(parser);
  }

  // Assign assembly-level labels to all label declarations.
  for (auto label : _labels) {
    std::string asmLabel = parser->getUnusedLabel(_name + "_" + label->name());
    label->setAsmLabel(asmLabel);
  }

  // Output assembly code for the rest of the statement types.
  for (auto statement : _statements) {
    statement->output(parser, shared_from_this(), endLabel);
  }

  // Unwind the stack, popping the saved registers, then return.
  // We have a label here so that when we have return statements
  // they can jump here without having to unwind the stack in
  // multiple places.
  parser->writeln(endLabel + ":");
  parser->writeInst("MOV SP FP");
  while (!_savedRegisters.empty()) {
    parser->writeInst("POP " + _savedRegisters.top());
    _savedRegisters.pop();
  }
  // If there are overflow parameters, pop them off the stack in
  // addition to jumping to the return address.
  if (0 < numOverflowParams) {
    parser->writeInst("RET " + toHexStr(numOverflowParams * DATA_SIZE, 2));
  } else {
    parser->writeInst("RET");
  }
}

/**
 * Translates the given source-level label within this function into the
 * assembly-level label that has been assigned to it. Returns the empty
 * string if the source-level label does not exist.
 */
std::string FunctionToken::toAsmLabel(const std::string& srcLabel) {
  for (auto label : _labels) {
    if (srcLabel == label->name()) {
      return label->getAsmLabel();
    }
  }
  return "";
}

/**
 * Gets the next statement and returns a pointer to it. Returns
 * a null pointer if there is no valid next statement.
 */
std::shared_ptr<StatementToken> StatementToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      std::vector<std::shared_ptr<GotoStatement>>& gotos,
      const std::shared_ptr<FunctionToken>& currentFunc,
      bool inLoop) {
  AtomToken t = tokenizer->peekNext();
  auto function = getFunction(t.str(), functions);
  if (t.str().empty()) {
    _error("Unexpected EOF.", t.line());
    return nullptr;
  } else if ("{" == t.str()) {
    std::shared_ptr<CompoundStatement> compound(new CompoundStatement());
    if (compound->parse(tokenizer, functions, globals, parameters,
                        localVars, labels, gotos, currentFunc, inLoop)) {
      return compound;
    }
  } else if ("if" == t.str()) {
    std::shared_ptr<IfStatement> ifStatement(new IfStatement());
    if (ifStatement->parse(tokenizer, functions, globals, parameters,
                           localVars, labels, gotos, currentFunc, inLoop)) {
      return ifStatement;
    }
  } else if ("for" == t.str()) {
    std::shared_ptr<ForStatement> forStatement(new ForStatement());
    if (forStatement->parse(tokenizer, functions, globals, parameters,
                            localVars, labels, gotos, currentFunc)) {
      return forStatement;
    }
  } else if ("while" == t.str()) {
    std::shared_ptr<WhileStatement> whileStatement(new WhileStatement());
    if (whileStatement->parse(tokenizer, functions, globals, parameters,
                              localVars, labels, gotos, currentFunc)) {
      return whileStatement;
    }
  } else if ("do" == t.str()) {
    std::shared_ptr<DoWhileStatement> doWhileStatement(new DoWhileStatement());
    if (doWhileStatement->parse(tokenizer, functions, globals, parameters,
                                localVars, labels, gotos, currentFunc)) {
      return doWhileStatement;
    }
  } else if ("break" == t.str()) {
    std::shared_ptr<BreakStatement> breakStatement(new BreakStatement());
    if (breakStatement->parse(tokenizer, inLoop)) {
      return breakStatement;
    }
  } else if ("continue" == t.str()) {
    std::shared_ptr<ContinueStatement> continueStatement(new ContinueStatement());
    if (continueStatement->parse(tokenizer, inLoop)) {
      return continueStatement;
    }
  } else if ("return" == t.str()) {
    std::shared_ptr<ReturnStatement> returnStatement(new ReturnStatement());
    if (returnStatement->parse(tokenizer, functions, globals, parameters,
                               localVars, currentFunc)) {
      return returnStatement;
    }
  } else if ("goto" == t.str()) {
    std::shared_ptr<GotoStatement> gotoStatement(new GotoStatement());
    if (gotoStatement->parse(tokenizer)) {
      gotos.push_back(gotoStatement);
      return gotoStatement;
    }
  } else if (";" == t.str()) {
    std::shared_ptr<NullStatement> nullStatement(new NullStatement());
    // Consume ';' token.
    tokenizer->getNext();
    return nullStatement;
  } else if (isLabelDeclaration(t.str())) {
    std::shared_ptr<LabelStatement> labelStatement(new LabelStatement());
    if (labelStatement->parse(tokenizer)) {
      labels.push_back(labelStatement);
      return labelStatement;
    }
  } else if (isType(t.str())) {
    std::shared_ptr<LocalVarToken> localVar(new LocalVarToken());
    if (localVar->parse(tokenizer, functions, globals, parameters, localVars)) {
      return localVar;
    }
  } else if (nullptr != function && "void" == function->type().name()) {
    std::shared_ptr<VoidStatement> voidStatement(new VoidStatement());
    if (voidStatement->parse(tokenizer, functions, globals, parameters,
                             localVars)) {
      return voidStatement;
    }
  } else {
    std::shared_ptr<ExprStatement> exprStatement(new ExprStatement());
    if (exprStatement->parse(tokenizer, functions, globals, parameters,
                             localVars)) {
      return exprStatement;
    }
  }
  return nullptr;
}

/**
 * The default implementation for a statement token's output, which
 * is to do nothing.
 */
void StatementToken::output(Parser *,
                            const std::shared_ptr<FunctionToken>&,
                            const std::string&,
                            const std::string&,
                            const std::string&) {
}

/**
 * Parses a compound statement, which consists of zero or more
 * statements within curly braces.
 */
bool CompoundStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      std::vector<std::shared_ptr<GotoStatement>>& gotos,
      const std::shared_ptr<FunctionToken>& currentFunc,
      bool inLoop) {
  _lineNum = tokenizer->peekNext().line();
  // Make sure the first token is a '{'.
  if (!_expect(tokenizer, "{")) {
    return false;
  }
  // Get the inner statements.
  while ("}" != tokenizer->peekNext().str()) {
    auto statement = StatementToken::parse(tokenizer, functions, globals,
                                           parameters, localVars, labels,
                                           gotos, currentFunc, inLoop);
    if (!statement) {
      return false;
    }
    // Local variables are only allowed as top level statements in
    // a function.
    if (std::dynamic_pointer_cast<LocalVarToken>(statement)) {
      _error("Local variables can only be declared as top level "
             "statements in a function.", statement->line());
      return false;
    }
    _statements.push_back(statement);
  }
  // Consume the final '}' token.
  tokenizer->getNext();
  return true;
}

void CompoundStatement::output(Parser *parser,
                               const std::shared_ptr<FunctionToken>& function,
                               const std::string& returnLabel,
                               const std::string& breakLabel,
                               const std::string& continueLabel) {
  for (auto statement : _statements) {
    statement->output(parser, function, returnLabel, breakLabel, continueLabel);
  }
}

bool LocalVarToken::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars) {
  _lineNum = tokenizer->peekNext().line();
  // Parse the type
  if (!_type.parse(tokenizer, functions, globals, parameters, localVars)) {
    return false;
  }
  // Validate the type
  if ("void" == _type.name()) {
    _error("Local var cannot be of type 'void'.", _type.line());
    return false;
  }
  // Get and validate the name
  AtomToken nameToken = tokenizer->getNext();
  if (nameToken.str().empty()) {
    _error("Unexpected EOF.", nameToken.line());
    return false;
  }
  _name = nameToken.str();
  if (!isValidName(_name)) {
    _error("Invalid local var name '" + _name + "'.", _type.line());
    return false;
  } else if (getFunction(_name, functions)) {
    _error("Local var '" + _name + "' conflicts with existing function name.",
           nameToken.line());
    return false;
  } else if (getGlobal(_name, globals)) {
    _error("Local var '" + _name + "' conflicts with existing global var name.",
           nameToken.line());
    return false;
  } else if (getParameter(_name, parameters)) {
    _error("Local var '" + _name + "' conflicts with existing parameter name.",
           nameToken.line());
    return false;
  } else if (getLocal(_name, localVars)) {
    _error("Local var '" + _name + "' conflicts with existing local var name.",
           nameToken.line());
    return false;
  }
  // Validate the value (if set)
  AtomToken next = tokenizer->getNext();
  AtomToken last = next;
  if ("=" == next.str()) {
    if (_type.isArray()) {
      // Make sure array expression has as many values as the type
      // requires, and make sure they are all constant.
      ArrayExprToken arrayExpr;
      if (!arrayExpr.parse(tokenizer, functions, globals,
                           parameters, localVars)) {
        return false;
      } else if (arrayExpr.size() != _type.arraySize()) {
        _error("Array size mismatch.", _lineNum);
        return false;
      }
      for (size_t i = 0; i < arrayExpr.size(); i++) {
        _initExprs.push_back(arrayExpr.get(i));
      }
    } else {
      // Not an array value, get the singleton initialization expression
      std::shared_ptr<ExprToken> expr(new ExprToken());
      if (!expr->parse(tokenizer, functions, globals,
                       parameters, localVars)) {
        return false;
      }
      _initExprs.push_back(expr);
    }
    // This should be a semicolon
    last = tokenizer->getNext();
  }
  if (last.empty()) {
    _error("Unexpected EOF.", last.line());
    return false;
  } else if (";" != last.str()) {
    _error("Unexpected token '" + last.str() + "', expected ';'.", last.line());
    return false;
  }
  return true;
}

/**
 * If an initial value is set for this local variable, this outputs the
 * assembly code to initialize the variable.
 */
void LocalVarToken::output(Parser *parser,
			   const std::shared_ptr<FunctionToken>&,
			   const std::string&,
			   const std::string&,
			   const std::string&) {
  // If this is an array, store the address of the data in the variable's
  // location.
  if (_type.isArray()) {
    // Store the location of the array at the variable's location.
    if (this->isReg()) {
      // Store FP + _dataOffset in the variable's register.
      parser->writeInst("MOV " + this->getReg() + " FP");
      if (0 != _dataOffset) {
        parser->writeInst("MOVI L " + toHexStr(_dataOffset));
        parser->writeInst("ADD " + this->getReg() + " L");
      }
    } else {
      // Store FP + _offset in M
      parser->writeInst("MOV M FP");
      if (0 != _offset) {
        parser->writeInst("MOVI L " + toHexStr(_offset));
        parser->writeInst("ADD M L");
      }
      // Store FP + _dataOffset in N
      parser->writeInst("MOV N FP");
      if (0 != _dataOffset) {
        parser->writeInst("MOVI L " + toHexStr(_dataOffset));
        parser->writeInst("ADD N L");
      }
      // Store the data offset in the variable's location relative to
      // the frame pointer.
      parser->writeInst("STOR N M");
    }
  }
  // If there is no initial value, do nothing else.
  if (_initExprs.empty()) {
    return;
  }
  // Store initial values.
  if (_type.isArray()) {
    for (size_t i = 0; i < _initExprs.size(); i++) {
      // Output initial expressions for each element.
      _initExprs[i]->output(parser, VarLocation(_dataOffset + (DATA_SIZE * i)));
    }
  } else {
    // Output initial expression for the scalar, to either the register
    // or frame pointer offset.
    _initExprs[0]->output(parser, VarLocation(_reg, _offset));
  }
}

/**
 * An expression statement has an expression followed by a semicolon.
 * This could include assignment, function calls, etc. Does not include
 * void function calls, they must be their own statement type, VoidStatement.
 */
bool ExprStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars) {
  _lineNum = tokenizer->peekNext().line();
  // An expression statement is just an expression followed by a semicolon.
  if (!_expr.parse(tokenizer, functions, globals, parameters, localVars)) {
    return false;
  }
  if (!_expect(tokenizer, ";")) {
    return false;
  }
  return true;
}

/**
 * Outputs assembly code for this expression statement. This just
 * evaluates the expression and then discards the result.
 */
void ExprStatement::output(Parser *parser,
                           const std::shared_ptr<FunctionToken>&,
                           const std::string&,
                           const std::string&,
                           const std::string&) {
  _expr.output(parser, VarLocation("L"));
}

/**
 * A void statement is a function call that returns void, followed by
 * a semicolon.
 */
bool VoidStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars) {
  _lineNum = tokenizer->peekNext().line();
  // Parse the function call.
  if (!_fnCall.parse(tokenizer, functions, globals, parameters, localVars)) {
    return false;
  }
  // Make sure the function call is void.
  auto function = getFunction(_fnCall.funcName(), functions);
  if (!function || "void" != function->type().name()) {
    _error("Expected function call to be of type 'void'.", _lineNum);
    return false;
  }
  // Make sure the next symbol is a semicolon.
  if (!_expect(tokenizer, ";")) {
    return false;
  }
  return true;
}

/**
 * Outputs the assembly code for this void statement.
 */
void VoidStatement::output(Parser *parser,
                           const std::shared_ptr<FunctionToken>&,
                           const std::string&,
                           const std::string&,
                           const std::string&) {
  _fnCall.output(parser);
}

/**
 * An if statement looks like "if (COND_EXPR) TRUE_STMT [else FALSE_STMT]",
 * where the else part is optional. COND_EXPR is an arbitrary expression,
 * and TRUE_STMT and FALSE_STMT are both arbitrary statements.
 */
bool IfStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      std::vector<std::shared_ptr<GotoStatement>>& gotos,
      const std::shared_ptr<FunctionToken>& currentFunc,
      bool inLoop) {
  _lineNum = tokenizer->peekNext().line();
  // Make sure it starts with "if"
  if (!_expect(tokenizer, "if")) {
    return false;
  }
  // Followed by an open parenthesis
  if (!_expect(tokenizer, "(")) {
    return false;
  }
  // Followed by a valid expression
  if (!_condExpr.parse(tokenizer, functions, globals, parameters, localVars)) {
    return false;
  }
  // Followed by a closing parenthesis
  if (!_expect(tokenizer, ")")) {
    return false;
  }
  // Followed by a valid statement
  _trueStatement = StatementToken::parse(tokenizer, functions, globals,
                                         parameters, localVars, labels,
                                         gotos, currentFunc, inLoop);
  if (!_trueStatement) {
    return false;
  }
  // If the next token is "else", check for the else statement.
  if ("else" == tokenizer->peekNext().str()) {
    _hasElse = true;
    // Consume the "else" token.
    tokenizer->getNext();
    // Make sure it is followed by a valid statement.
    _falseStatement = StatementToken::parse(tokenizer, functions, globals,
                                            parameters, localVars, labels,
                                            gotos, currentFunc, inLoop);
    if (!_falseStatement) {
      return false;
    }
  } else {
    _hasElse = false;
  }
  return true;
}

/**
 * Outputs the assembly code for this if statement.
 */
void IfStatement::output(Parser *parser,
                         const std::shared_ptr<FunctionToken>& function,
                         const std::string& returnLabel,
                         const std::string& breakLabel,
                         const std::string& continueLabel) {
  std::string falseLabel = parser->getUnusedLabel(function->name() + "_if_false");
  std::string endLabel = parser->getUnusedLabel(function->name() + "_if_end");
  // Test the condition and jump to the false label if it is false.
  _condExpr.output(parser, VarLocation("L"));
  parser->writeInst("TST L L");
  parser->writeInst("JEQ " + falseLabel);
  // Output the true statement and jump to the end.
  _trueStatement->output(parser, function, returnLabel, breakLabel,
                         continueLabel);
  parser->writeInst("JMPI " + endLabel);
  // Output the false statement label and the false statement.
  parser->writeln(falseLabel + ":");
  if (nullptr != _falseStatement) {
    _falseStatement->output(parser, function, returnLabel, breakLabel,
                            continueLabel);
  }
  // Output the end label that the true statement uses to jump over the
  // false statement.
  parser->writeln(endLabel + ":");
}

/**
 * A for-statement is of the form:
 * "for (INIT_LIST; COND_EXPR; LOOP_LIST) STMT"
 * Where INIT_LIST and LOOP_LIST are comma-separated lists of
 * expressions, COND_EXPR is a single expression, and STMT is
 * an arbitrary statement.
 */
bool ForStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      std::vector<std::shared_ptr<GotoStatement>>& gotos,
      const std::shared_ptr<FunctionToken>& currentFunc) {
  _lineNum = tokenizer->peekNext().line();
  // Start with the "for" keyword.
  if (!_expect(tokenizer, "for")) {
    return false;
  }
  // Next get the "(" token.
  if (!_expect(tokenizer, "(")) {
    return false;
  }
  // Get the INIT_LIST.
  if (";" != tokenizer->peekNext().str()) {
    while (true) {
      std::shared_ptr<ExprToken> expr(new ExprToken());
      if (!expr->parse(tokenizer, functions, globals, parameters, localVars)) {
        return false;
      }
      _initExprs.push_back(expr);
      // If the next token is not a comma, we are done. Otherwise consume
      // the comma and continue.
      if ("," != tokenizer->peekNext().str()) {
        break;
      }
      tokenizer->getNext();
    }
  }
  // Make sure the INIT_LIST ended with a ';'.
  if (!_expect(tokenizer, ";")) {
    return false;
  }
  // Get the COND_EXPR. If the next token is a ';', then the COND_EXPR
  // is an implicit truthy value.
  if (";" == tokenizer->peekNext().str()) {
    _condExpr = std::shared_ptr<ExprToken>(new ExprToken(1));
  } else {
    _condExpr = std::shared_ptr<ExprToken>(new ExprToken());
    if (!_condExpr->parse(tokenizer, functions, globals,
                          parameters, localVars)) {
      return false;
    }
  }
  // Make sure the COND_EXPR ended with a ';'.
  if (!_expect(tokenizer, ";")) {
    return false;
  }
  // Get the LOOP_LIST.
  if (")" != tokenizer->peekNext().str()) {
    while (true) {
      std::shared_ptr<ExprToken> expr(new ExprToken());
      if (!expr->parse(tokenizer, functions, globals, parameters, localVars)) {
        return false;
      }
      _loopExprs.push_back(expr);
      // If the next token is not a comma, we are done. Otherwise consume
      // the comma and continue.
      if ("," != tokenizer->peekNext().str()) {
        break;
      }
      tokenizer->getNext();
    }
  }
  // Make sure the LOOP_LIST ended with a ')'.
  if (!_expect(tokenizer, ")")) {
    return false;
  }
  // Get the statement that is the body of the loop, make sure it is valid.
  _body = StatementToken::parse(tokenizer, functions, globals, parameters,
                                localVars, labels, gotos, currentFunc, true);
  if (!_body) {
    return false;
  }
  return true;
}

/**
 * Outputs the assembly code for this for statement. The output looks like
 * the following:
 *
 * [initial-expressions]
 * start_label:
 * if (![condition])
 *   goto break_label;
 * [body-statements]
 * continue_label:
 * [loop-expressions]
 * goto start_label;
 * break_label:
 */
void ForStatement::output(Parser *parser,
                          const std::shared_ptr<FunctionToken>& function,
                          const std::string& returnLabel,
                          const std::string&,
                          const std::string&) {
  // Evaluate the initial expressions and discard the result.
  for (auto expr : _initExprs) {
    expr->output(parser, VarLocation("L"));
  }
  // Create the start, break, and continue labels.
  std::string startLabel =
    parser->getUnusedLabel(function->name() + "_for_start");
  std::string breakLabel =
    parser->getUnusedLabel(function->name() + "_for_break");
  std::string continueLabel =
    parser->getUnusedLabel(function->name() + "_for_continue");
  // Output the start label and test the condition.
  parser->writeln(startLabel + ":");
  _condExpr->output(parser, VarLocation("L"));
  parser->writeInst("TST L L");
  parser->writeInst("JEQ " + breakLabel);
  // Output the function body followed by the continue label.
  _body->output(parser, function, returnLabel, breakLabel, continueLabel);
  parser->writeln(continueLabel + ":");
  // Output the loop expressions and jump to the start of the loop.
  for (auto expr : _loopExprs) {
    expr->output(parser, VarLocation("L"));
  }
  parser->writeInst("JMPI " + startLabel);
  // Output the break label.
  parser->writeln(breakLabel + ":");
}

/**
 * While statements are of the form:
 * "while (COND_EXPR) STMT"
 * where COND_EXPR is an arbitrary expression and STMT is an
 * arbitrary statement.
 */
bool WhileStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      std::vector<std::shared_ptr<GotoStatement>>& gotos,
     const std::shared_ptr<FunctionToken>& currentFunc) {
  _lineNum = tokenizer->peekNext().line();
  // First token should be the "while" keyword.
  if (!_expect(tokenizer, "while")) {
    return false;
  }
  // Next token should be an open parenthesis.
  if (!_expect(tokenizer, "(")) {
    return false;
  }
  // Now we parse the conditional expression.
  _condExpr = std::shared_ptr<ExprToken>(new ExprToken());
  if (!_condExpr->parse(tokenizer, functions, globals,
                        parameters, localVars)) {
    return false;
  }
  // Next token should be a closing parenthesis.
  if (!_expect(tokenizer, ")")) {
    return false;
  }
  // Now we make sure the statement exists and is valid.
  _body = StatementToken::parse(tokenizer, functions, globals,
                                parameters, localVars, labels, gotos,
                                currentFunc, true);
  if (!_body) {
    return false;
  }
  return true;
}

/**
 * Outputs the assembly code for this while statement. This should be of
 * the following form:
 *
 * continue_label:
 * if (![condition])
 *   goto break_label;
 * [body-statements]
 * goto continue_label;
 * break_label:
 */
void WhileStatement::output(Parser *parser,
                            const std::shared_ptr<FunctionToken>& function,
                            const std::string& returnLabel,
                            const std::string&,
                            const std::string&) {
  // Create the break and continue labels.
  std::string breakLabel =
    parser->getUnusedLabel(function->name() + "_while_break");
  std::string continueLabel =
    parser->getUnusedLabel(function->name() + "_while_continue");
  // Output the continue label and test the condition.
  parser->writeln(continueLabel + ":");
  _condExpr->output(parser, VarLocation("L"));
  parser->writeInst("TST L L");
  parser->writeInst("JEQ " + breakLabel);
  // Output the loop body and then jump to the start again.
  _body->output(parser, function, returnLabel, breakLabel, continueLabel);
  parser->writeInst("JMPI " + continueLabel);
  // Output the break label.
  parser->writeln(breakLabel + ":");
}

/**
 * Do-while statements are of the form:
 * "do STMT while (COND_EXPR);"
 * where COND_EXPR is an arbitrary expression and STMT is an
 * arbitrary statement.
 */
bool DoWhileStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      std::vector<std::shared_ptr<GotoStatement>>& gotos,
      const std::shared_ptr<FunctionToken>& currentFunc) {
  _lineNum = tokenizer->peekNext().line();
  // Make sure the first token is the "do" keyword.
  if (!_expect(tokenizer, "do")) {
    return false;
  }
  // Make sure the body exists and is valid.
  _body = StatementToken::parse(tokenizer, functions, globals,
                                parameters, localVars, labels, gotos,
                                currentFunc, true);
  if (!_body) {
    return false;
  }
  // Next token should be the "while" keyword.
  if (!_expect(tokenizer, "while")) {
    return false;
  }
  // Next token should be an opening parenthesis.
  if (!_expect(tokenizer, "(")) {
    return false;
  }
  // Next we should be able to parse the expression.
  _condExpr = std::shared_ptr<ExprToken>(new ExprToken());
  if (!_condExpr->parse(tokenizer, functions, globals,
                        parameters, localVars)) {
    return false;
  }
  // Next two tokens should be a closing parenthesis followed by
  // a semicolon.
  if (!_expect(tokenizer, ")")) {
    return false;
  }
  if (!_expect(tokenizer, ";")) {
    return false;
  }
  return true;
}

/**
 * Outputs the assembly code for this do-while statement. This should be
 * of the form:
 *
 * continue_label:
 * [body-statments]
 * if ([condition])
 *   goto continue_label;
 * break_label:
 */
void DoWhileStatement::output(Parser *parser,
                              const std::shared_ptr<FunctionToken>& function,
                              const std::string& returnLabel,
                              const std::string&,
                              const std::string&) {
  // Create the break and continue labels.
  std::string breakLabel =
    parser->getUnusedLabel(function->name() + "_do_while_break");
  std::string continueLabel =
    parser->getUnusedLabel(function->name() + "_do_while_continue");
  // Output the continue label.
  parser->writeln(continueLabel + ":");
  // Output the loop body.
  _body->output(parser, function, returnLabel, breakLabel, continueLabel);
  // Test the condition.
  _condExpr->output(parser, VarLocation("L"));
  parser->writeInst("TST L L");
  parser->writeInst("JNE " + continueLabel);
  // Output the break label.
  parser->writeln(breakLabel + ":");
}

/**
 * A break statement should just be the "break" token followed
 * by a ";" token. If we are not in a loop, that is an error.
 */
bool BreakStatement::parse(Tokenizer *tokenizer, bool inLoop) {
  _lineNum = tokenizer->peekNext().line();
  if (!_expect(tokenizer, "break")) {
    return false;
  } else if (!_expect(tokenizer, ";")) {
    return false;
  } else if (!inLoop) {
    _error("Must be within a loop statement to use 'break;'.", _lineNum);
    return false;
  }
  return true;
}

/**
 * Outputs this break statement.
 */
void BreakStatement::output(Parser *parser,
                            const std::shared_ptr<FunctionToken>&,
                            const std::string&,
                            const std::string& breakLabel,
                            const std::string&) {
  parser->writeInst("JMPI " + breakLabel);
}

/**
 * A continue statement should just be the "continue" token followed
 * by a ";" token. If we are not in a loop, that is an error.
 */
bool ContinueStatement::parse(Tokenizer *tokenizer, bool inLoop) {
  _lineNum = tokenizer->peekNext().line();
  if (!_expect(tokenizer, "continue")) {
    return false;
  } else if (!_expect(tokenizer, ";")) {
    return false;
  } else if (!inLoop) {
    _error("Must be within a loop statement to use 'continue;'.", _lineNum);
    return false;
  }
  return true;
}

/**
 * Outputs this continue statement.
 */
void ContinueStatement::output(Parser *parser,
                               const std::shared_ptr<FunctionToken>&,
                               const std::string&,
                               const std::string&,
                               const std::string& continueLabel) {
  parser->writeInst("JMPI " + continueLabel);
}

/**
 * A return statement is of the form:
 * "return [EXPR] ;"
 * where EXPR is an optional arbitrary expression. If we are within
 * a void function, EXPR must not be included. If we are within a
 * non-void function, EXPR must be included.
 */
bool ReturnStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      const std::shared_ptr<FunctionToken>& currentFunc) {
  _lineNum = tokenizer->peekNext().line();
  // Make sure the first token is the "return" keyword.
  if (!_expect(tokenizer, "return")) {
    return false;
  }
  // If the next token is not a semicolon, parse the expression.
  if (";" != tokenizer->peekNext().str()) {
    _hasExpr = true;
    if (!_returnExpr.parse(tokenizer, functions, globals,
                           parameters, localVars)) {
      return false;
    }
  } else {
    _hasExpr = false;
  }
  // Check for the trailing semicolon.
  if (!_expect(tokenizer, ";")) {
    return false;
  }
  // Make sure that _hasExpr matches up with the void-ness of the
  // containing function.
  if (_hasExpr && "void" == currentFunc->type().name()) {
    _error("Cannot return a value from a void function.", _lineNum);
    return false;
  } else if (!_hasExpr && "void" != currentFunc->type().name()) {
    _error("Return statement must include a value in a non-void function.",
           _lineNum);
    return false;
  }
  return true;
}

/**
 * Calculates the return value from the return expression, if the
 * function is not void. Then jumps to the epilogue of the function.
 * Return values are stored in the register L.
 */
void ReturnStatement::output(Parser *parser,
                             const std::shared_ptr<FunctionToken>&,
                             const std::string& returnLabel,
                             const std::string&,
                             const std::string&) {
  if (_hasExpr) {
    _returnExpr.output(parser, VarLocation("L"));
  }
  parser->writeInst("JMPI " + returnLabel);
}

/**
 * Parses a label declaration like "label:". Must start with a name and
 * end with a colon, without whitespace.
 */
bool LabelStatement::parse(Tokenizer *tokenizer) {
  _lineNum = tokenizer->peekNext().line();
  // Make sure it is a valid label declaration.
  AtomToken t = tokenizer->getNext();
  if (t.str().empty()) {
    _error("Unexpected EOF.", t.line());
    return false;
  } else if (!isLabelDeclaration(t.str())) {
    _error("Invalid label declaration.", t.line());
    return false;
  }
  // Name is the label declaration minus the trailing colon.
  _name = t.str().substr(0, t.str().size() - 1);
  return true;
}

/**
 * Outputs the assembly-level label for this label statement.
 */
void LabelStatement::output(Parser *parser,
                            const std::shared_ptr<FunctionToken>&,
                            const std::string&,
                            const std::string&,
                            const std::string&) {
  parser->writeln(_asmLabel + ":");
}

/**
 * A goto statement is of the form "goto LABEL;".
 */
bool GotoStatement::parse(Tokenizer *tokenizer) {
  _lineNum = tokenizer->peekNext().line();
  // Make sure the first token is the "goto" keyword.
  if (!_expect(tokenizer, "goto")) {
    return false;
  }
  // Get the label.
  AtomToken labelToken = tokenizer->getNext();
  if (labelToken.str().empty()) {
    _error("Unexpected EOF.", labelToken.line());
    return false;
  } else if (!isValidName(labelToken.str())) {
    _error("Invalid label name in goto statement.", labelToken.line());
    return false;
  }
  _label = labelToken.str();
  // Get the trailing ";" token.
  if (!_expect(tokenizer, ";")) {
    return false;
  }
  return true;
}

/**
 * Outputs the assembly code for this goto statement.
 */
void GotoStatement::output(Parser *parser,
                           const std::shared_ptr<FunctionToken>& function,
                           const std::string&,
                           const std::string&,
                           const std::string&) {
  std::string asmLabel = function->toAsmLabel(_label);
  parser->writeInst("JMPI " + asmLabel);
}
