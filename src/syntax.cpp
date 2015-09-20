/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include <regex>
#include "tokenizer.h"
#include "syntax.h"

/**
 * Returns the opposing paranthesis for (), [], or {} pairs.
 * Returns an empty string if the input is not one of the above.
 */
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

/**
 * Returns true if the given string is a valid name for a function,
 * variable, etc. A valid name starts with an alphabetic or underscore
 * character, and is followed by zero or more alphanumeric or
 * underscore characters.
 */
bool isValidName(const std::string& name) {
  return std::regex_match(name, std::regex("^[_a-zA-Z][_a-zA-Z0-9]*$"));
}

/**
 * Returns true if the given string is a valid label declaration.
 * A label declaration starts with a valid name and is followed by
 * a colon.
 */
bool isLabelDeclaration(const std::string& label) {
  return std::regex_match(label, std::regex("^[_a-zA-Z][_a-zA-Z0-9]*:$"));
}

/**
 * Searches through a vector of parameters and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<ParamToken> getParameter(
      const std::string& name,
      const std::vector<std::shared_ptr<ParamToken>>& parameters) {
  for (auto param : parameters) {
    if (param->name() == name) {
      return param;
    }
  }
  return nullptr;
}

/**
 * Searches through a vector of local variables and returns the one that
 * matches the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<LocalVarToken> getLocal(
      const std::string& name,
      const std::vector<std::shared_ptr<LocalVarToken>>& locals) {
  for (auto local : locals) {
    if (local->name() == name) {
      return local;
    }
  }
  return nullptr;
}

/**
 * Searches through a vector of functions and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<FunctionToken> getFunction(
      const std::string& name,
      const std::vector<std::shared_ptr<FunctionToken>>& functions) {
  for (auto func : functions) {
    if (func->name() == name) {
      return func;
    }
  }
  return nullptr;
}

/**
 * Searches through a vector of globals and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<GlobalVarToken> getGlobal(
     const std::string& name,
     const std::vector<std::shared_ptr<GlobalVarToken>>& globals) {
  for (auto global : globals) {
    if (global->name() == name) {
      return global;
    }
  }
  return nullptr;
}

/**
 * Returns true if the given string names a valid type. There
 * are only a few valid types right now so this function
 * is a bit crude.
 */
bool isType(const std::string& type) {
  return "void" == type ||  "uint16" == type;
}

/**
 * Prints an error message with the given line number.
 */
void _error(const std::string& msg, int lineNum) {
  // TODO: It would be nice to have error messages that showed you
  // the source line and pointed to the token causing an error.
  std::cerr << "Error:" << lineNum << ": " << msg << std::endl;
}

/**
 * Prints a warning message with the given line number.
 */
void _warn(const std::string& msg, int lineNum) {
  std::cerr << "Warning:" << lineNum << ": " << msg << std::endl;
}

/**
 * Consumes the next token, and prints an error message and returns
 * false if it finds EOF or a token other than the one it was
 * expecting. Returns true if the next token was the expected token.
 */
bool _expect(Tokenizer *tokenizer, const std::string& str, bool errors = true) {
  AtomToken t = tokenizer->getNext();
  if (t.str().empty()) {
    if (errors) {
      _error("Unexpected EOF, expected '" + str + "'.", t.line());
    }
    return false;
  } else if (str != t.str()) {
    if (errors) {
      _error("Unexpected token '" + t.str() + "', expected '" + str + "'.",
             t.line());
    }
    return false;
  }
  return true;
}

/**
 * Parses a literal value in the code, such as "0x00ff" or "1234".
 * Only hex and decimal formats supported currently, no string
 * literals. Only stores up to the maximum value of a signed int.
 * Returns false if the literal is not in a valid format.
 */
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
        auto topOp = std::dynamic_pointer_cast<OperatorToken>(opStack.top());
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
        // TODO: handle function calls here
        _error("Function calls not yet implemented, "
               "invalid token '" + t.str() + "'.",
               t.line());
        return false;
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
  return true;
}

/**
 * Validate the assignments in the expression, make sure that we only
 * assign to types that can be assigned to.
 */
bool ExprToken::_validate() {
  std::stack<std::string> operands;
  for (auto token : _postfix) {
    if (typeid(*token) == typeid(LiteralToken)) {
      operands.push("rvalue");
    } else if (typeid(*token) == typeid(GlobalVarToken) ||
               typeid(*token) == typeid(ParamToken) ||
               typeid(*token) == typeid(LocalVarToken)) {
      operands.push("lvalue");
    } else if (typeid(*token) == typeid(OperatorToken)) {
      auto op = std::dynamic_pointer_cast<OperatorToken>(token);
      std::string rhs = operands.top();
      operands.pop();
      std::string lhs;
      if (op->isBinary()) {
        lhs = operands.top();
        operands.pop();
      }
      std::string result;
      if ("=" == op->str()) {
        if ("lvalue" != lhs) {
          _error("Can't assign to an rvalue in expression.", op->line());
          return false;
        }
        result = "rvalue";
      } else if ("*" == op->str() && op->isUnary()) {
        result = "lvalue";
      } else if ("&" == op->str() && op->isUnary()) {
        if ("lvalue" != rhs) {
          _error("Can't get address of an rvalue in expression.", op->line());
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
    if (typeid(*token) == typeid(LiteralToken) ||
        typeid(*token) == typeid(GlobalVarToken)) {
      operands.push(token);
    } else if (typeid(*token) == typeid(ParamToken) ||
               typeid(*token) == typeid(LocalVarToken)) {
      // We don't know parameter or local variable values at
      // compile time, so this expression can't be constant.
      _const = false;
      return;
    } else if (typeid(*token) == typeid(OperatorToken)) {
      auto op = std::dynamic_pointer_cast<OperatorToken>(token);
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
        if (typeid(*lhs) != typeid(GlobalVarToken)) {
          _const = false;
          return;
        }
        auto global = std::dynamic_pointer_cast<GlobalVarToken>(lhs);
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
 */
bool FunctionToken::parse(
      Tokenizer *tokenizer,
      std::vector<std::shared_ptr<FunctionToken>>& functions,
      std::vector<std::shared_ptr<GlobalVarToken>>& globals) {
  _lineNum = _type.line();
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
  // Get the statements within the function body.
  while ("}" != tokenizer->peekNext().str()) {
    auto statement = StatementToken::parse(tokenizer, functions, globals,
                                           _parameters, _localVars, _labels,
                                           shared_from_this());
    // If the statement is valid, add it to the list of statements.
    if (!statement) {
      return false;
    }
    _statements.push_back(statement);
    // If it's a local variable declaration, add it to the list of local
    // variables.
    if (typeid(*statement) == typeid(LocalVarToken)) {
      _localVars.push_back(std::dynamic_pointer_cast<LocalVarToken>(statement));
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
  return true;
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
      const std::shared_ptr<FunctionToken>& currentFunc,
      bool inLoop) {
  AtomToken t = tokenizer->peekNext();
  if (t.str().empty()) {
    _error("Unexpected EOF.", t.line());
    return nullptr;
  } else if ("{" == t.str()) {
    std::shared_ptr<CompoundStatement> compound(new CompoundStatement());
    if (compound->parse(tokenizer, functions, globals, parameters,
                        localVars, labels, currentFunc, inLoop)) {
      return compound;
    }
  } else if ("if" == t.str()) {
    std::shared_ptr<IfStatement> ifStatement(new IfStatement());
    if (ifStatement->parse(tokenizer, functions, globals, parameters,
                           localVars, labels, currentFunc, inLoop)) {
      return ifStatement;
    }
  } else if ("for" == t.str()) {
    std::shared_ptr<ForStatement> forStatement(new ForStatement());
    if (forStatement->parse(tokenizer, functions, globals, parameters,
                            localVars, labels, currentFunc)) {
      return forStatement;
    }
  } else if ("while" == t.str()) {
    std::shared_ptr<WhileStatement> whileStatement(new WhileStatement());
    if (whileStatement->parse(tokenizer, functions, globals, parameters,
                              localVars, labels, currentFunc)) {
      return whileStatement;
    }
  } else if ("do" == t.str()) {
    std::shared_ptr<DoWhileStatement> doWhileStatement(new DoWhileStatement());
    if (doWhileStatement->parse(tokenizer, functions, globals, parameters,
                                localVars, labels, currentFunc)) {
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
    if (gotoStatement->parse(tokenizer, labels)) {
      return gotoStatement;
    }
  } else if (";" == t.str()) {
    std::shared_ptr<NullStatement> nullStatement(new NullStatement());
    // Consume ';' token.
    tokenizer->getNext();
    return nullStatement;
  } else if (isLabelDeclaration(t.str())) {
    std::shared_ptr<LabelStatement> labelStatement(new LabelStatement());
    if (labelStatement->parse(tokenizer, labels)) {
      return labelStatement;
    }
  } else if (isType(t.str())) {
    std::shared_ptr<LocalVarToken> localVar(new LocalVarToken());
    if (localVar->parse(tokenizer, functions, globals, parameters, localVars)) {
      return localVar;
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

bool CompoundStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
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
                                           currentFunc, inLoop);
    if (!statement) {
      return false;
    }
    // Local variables are only allowed as top level statements in
    // a function.
    if (typeid(*statement) == typeid(LocalVarToken)) {
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
      if (!expr->parse(tokenizer, functions, globals)) {
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
                                         currentFunc, inLoop);
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
                                            currentFunc, inLoop);
    if (!_falseStatement) {
      return false;
    }
  } else {
    _hasElse = false;
  }
  return true;
}

bool ForStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      const std::shared_ptr<FunctionToken>& currentFunc) {
  // TODO: Parse for statements
  _lineNum = tokenizer->peekNext().line();
  tokenizer = tokenizer;
  functions.size();
  globals.size();
  parameters.size();
  labels.size();
  localVars.size();
  currentFunc->name();
  _error("For statement not yet implemented.", _lineNum);
  return false;
}

bool WhileStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      const std::shared_ptr<FunctionToken>& currentFunc) {
  // TODO: Parse while statements
  _lineNum = tokenizer->peekNext().line();
  tokenizer = tokenizer;
  functions.size();
  globals.size();
  parameters.size();
  labels.size();
  localVars.size();
  currentFunc->name();
  _error("While statement not yet implemented.", _lineNum);
  return false;
}

bool DoWhileStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      std::vector<std::shared_ptr<LabelStatement>>& labels,
      const std::shared_ptr<FunctionToken>& currentFunc) {
  // TODO: Parse do-while statements
  _lineNum = tokenizer->peekNext().line();
  tokenizer = tokenizer;
  functions.size();
  globals.size();
  parameters.size();
  labels.size();
  localVars.size();
  currentFunc->name();
  _error("Do-while statement not yet implemented.", _lineNum);
  return false;
}

bool BreakStatement::parse(Tokenizer *tokenizer, bool inLoop) {
  // TODO: Parse break statements
  _lineNum = tokenizer->peekNext().line();
  tokenizer = tokenizer;
  inLoop = inLoop;
  _error("Break statement not yet implemented.", _lineNum);
  return false;
}

bool ContinueStatement::parse(Tokenizer *tokenizer, bool inLoop) {
  // TODO: Parse continue statements
  _lineNum = tokenizer->peekNext().line();
  tokenizer = tokenizer;
  inLoop = inLoop;
  _error("Continue statement not yet implemented.", _lineNum);
  return false;
}

bool ReturnStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<FunctionToken>>& functions,
      const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
      const std::vector<std::shared_ptr<ParamToken>>& parameters,
      const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
      const std::shared_ptr<FunctionToken>& currentFunc) {
  // TODO: Parse return statements
  _lineNum = tokenizer->peekNext().line();
  tokenizer = tokenizer;
  functions.size();
  globals.size();
  parameters.size();
  localVars.size();
  currentFunc->name();
  _error("Return statement not yet implemented.", _lineNum);
  return false;
}

bool LabelStatement::parse(
      Tokenizer *tokenizer,
      std::vector<std::shared_ptr<LabelStatement>>& labels) {
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
  // Add to the function's list of labels.
  labels.push_back(shared_from_this());
  return true;
}

bool GotoStatement::parse(
      Tokenizer *tokenizer,
      const std::vector<std::shared_ptr<LabelStatement>>& labels) {
  // TODO: Parse goto statements
  _lineNum = tokenizer->peekNext().line();
  tokenizer = tokenizer;
  labels.size();
  _error("Goto statement not yet implemented.", _lineNum);
  return false;
}
