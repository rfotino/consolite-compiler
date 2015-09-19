/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include <regex>
#include "tokenizer.h"
#include "syntax.h"

std::string otherParen(const std::string& paren) {
  if ("(" == paren) {
    return ")";
  } else if (")" == paren) {
    return "(";
  } else if ("[" == paren) {
    return "]";
  } else if ("]" == paren) {
    return "[";
  } else if ("{" == paren) {
    return "}";
  } else if ("}" == paren) {
    return "{";
  }
  return "";
}

bool isValidName(const std::string& name) {
  return std::regex_match(name, std::regex("^[_a-zA-Z][_a-zA-Z0-9]*$"));
}

bool isFunctionName(const std::string& name,
                    const std::vector<FunctionToken>& functions) {
  for (auto func : functions) {
    if (func.name() == name) {
      return true;
    }
  }
  return false;
}

bool isGlobalName(const std::string& name,
                  const std::vector<GlobalVarToken>& globals) {
  for (auto global : globals) {
    if (global.name() == name) {
      return true;
    }
  }
  return false;
}

bool isType(const std::string& type) {
  return "void" == type || "int16" == type || "uint16" == type;
}

void Token::_error(const std::string& msg, int lineNum) {
  // TODO: It would be nice to have error messages that showed you
  // the source line and pointed to the token causing an error.
  std::cerr << "Error:" << lineNum << ": " << msg << std::endl;
}

bool LiteralToken::parse(const AtomToken& token) {
  _lineNum = token.line();
  if (std::regex_match(token.str(), std::regex("^0[xX][0-9a-fA-F]+$"))) {
    _value = 0;
    for (size_t i = 2; i < token.str().size(); i++) {
      char c = token.str().at(i);
      uint8_t hexVal =
        ('0' <= c && c <= '9') ? (c - '0') :
        ('a' <= c && c <= 'f') ? (c - 'a' + 10) : (c - 'A' + 10);
      _value = (_value * 16) + hexVal;
    }
  } else if (std::regex_match(token.str(), std::regex("^[0-9]+$"))) {
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

int OperatorToken::operate(int lhs, int rhs) const {
  if (isUnary()) {
    if ("-" == _op) {
      return -rhs;
    } else if ("*" == _op) {
      throw "No dereferencing allowed in constant expressions.";
    } else if ("&" == _op) {
      throw "No address-of allowed in constant expressions.";
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
	throw "Division by zero.";
      }
      return lhs / rhs;
    } else if ("%" == _op) {
      if (0 == rhs) {
	throw "Division by zero.";
      }
      return lhs % rhs;
    } else if ("=" == _op) {
      throw "Assignment not yet supported.";
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
  throw "Invalid operator in expression";
}

bool TypeToken::parse(Tokenizer *tokenizer,
                      std::vector<FunctionToken> &functions,
                      std::vector<GlobalVarToken> &globals) {
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
    if (!expr.parse(tokenizer, functions, globals)) {
      return false;
    }
    if (!expr.isConst()) {
      _error("Array size must be known at compile time.", expr.line());
      return false;
    }
    _arraySize = expr.val();
    AtomToken endBracket = tokenizer->getNext();
    if ("]" != endBracket.str()) {
      _error("Unexpected token '" + endBracket.str() + "', expected ']'.",
             endBracket.line());
      return false;
    }
  }
  return true;
}

bool ExprToken::parse(Tokenizer *tokenizer,
                      std::vector<FunctionToken> &functions,
                      std::vector<GlobalVarToken> &globals) {
  functions = functions;
  globals = globals;

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
             typeid(*opStack.top()) == typeid(OperatorToken)) {
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
      } else if (op->maybeUnary() && (prev.empty() || "op" == prev)) {
        op->setUnary();
      } else {
        _error("Unexpected token '" + t.str() + "' in expression.", t.line());
        return false;
      }
      // Handle the operator stack based on precedence.
      while (!opStack.empty() &&
             typeid(*opStack.top()) == typeid(OperatorToken)) {
        std::shared_ptr<OperatorToken> topOp = std::dynamic_pointer_cast<OperatorToken>(opStack.top());
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
      // TODO: handle names (variables and functions) here
      _error("Names not yet implemented, name = '" + t.str() + "'.", t.line());
      return false;
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
  return true;
}

int ExprToken::val() const {
  if (!_const) {
    return 0;
  }
  // Evaluate the postfix expression
  std::stack<int> operands;
  for (auto token : _postfix) {
    if (typeid(*token) == typeid(LiteralToken)) {
      operands.push(token->val());
    } else if (typeid(*token) == typeid(OperatorToken)) {
      auto op = std::dynamic_pointer_cast<OperatorToken>(token);
      int rhs = operands.top();
      operands.pop();
      int lhs;
      if (op->isBinary()) {
        lhs = operands.top();
        operands.pop();
      } else {
        lhs = 0;
      }
      int result = op->operate(lhs, rhs);
      operands.push(result);
    }
  }
  return operands.top();
}

bool ArrayExprToken::parse(Tokenizer *tokenizer,
                           std::vector<FunctionToken> &functions,
                           std::vector<GlobalVarToken> &globals) {
  // Make sure the first token is a '{' symbol
  AtomToken first = tokenizer->getNext();
  if (first.empty()) {
    _error("Unexpected EOF.", first.line());
    return false;
  } else if ("{" != first.str()) {
    _error("Unexpected token '" + first.str() + "', expected '{'.",
           first.line());
    return false;
  }
  _lineNum = first.line();
  // Check if the next token is a closing brace, in which
  // case we don't need to check for expressions.
  if ("}" == tokenizer->peekNext().str()) {
    tokenizer->getNext();
    return true;
  }
  // Get any expressions we find, separated by commas
  while (true) {
    ExprToken expr;
    if (!expr.parse(tokenizer, functions, globals)) {
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

bool GlobalVarToken::parse(Tokenizer *tokenizer,
                           std::vector<FunctionToken> &functions,
                           std::vector<GlobalVarToken> &globals) {
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
  } else if (isFunctionName(_name, functions)) {
    _error("Global var '" + _name + "' conflicts with existing function name.",
           _type.line());
    return false;
  } else if (isGlobalName(_name, globals)) {
    _error("Global var '" + _name + "' conflicts with existing gobal var name.",
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
      for (int i = 0; i < arrayExpr.size(); i++) {
        ExprToken expr = arrayExpr.get(i);
        if (!expr.isConst()) {
          _error("Global value must be known at compile time.", expr.line());
          return false;
        }
        _arrayValues.push_back(expr.val());
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
      for (int i = 0; i < _type.arraySize(); i++) {
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

bool FunctionToken::parse(Tokenizer *tokenizer,
                          std::vector<FunctionToken> &functions,
                          std::vector<GlobalVarToken> &globals) {
  tokenizer = tokenizer;
  functions = functions;
  globals = globals;
  return true;
}
