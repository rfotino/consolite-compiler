/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include <regex>
#include "tokenizer.h"
#include "syntax.h"

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

bool TypeToken::parse(Tokenizer *tokenizer,
                      std::vector<FunctionToken> &functions,
                      std::vector<GlobalVarToken> &globals) {
  AtomToken typeName = tokenizer->getNext();
  if (!isType(typeName.str())) {
    std::cerr << "Error: Invalid type '" << typeName.str() << "' on line "
              << typeName.line() << "." << std::endl;
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
      std::cerr << "Error: Array size must be known at compile time on line "
                << expr.line() << "." << std::endl;
      return false;
    }
    _arraySize = expr.val();
    AtomToken endBracket = tokenizer->getNext();
    if ("]" != endBracket.str()) {
      std::cerr << "Error: Unexpected token '" << endBracket.str()
                << "', expected ']' on line " << endBracket.line()
                << "." << std::endl;
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
  while (true) {
    AtomToken t = tokenizer->peekNext();
    std::shared_ptr<LiteralToken> literal(new LiteralToken());
    if ("(" == t.str() || ")" == t.str()) {
      std::cerr << "Error: Parentheses not implemented yet on line "
                << t.line() << "." << std::endl;
      return false;
    } else if (literal->parse(t)) {
      _root = literal;
      tokenizer->getNext();
      return true;
    } else if (isValidName(t.str())) {
      std::cerr << "Error: Names not implemented yet on line "
                << t.line() << ", name = '" << t.str() << "'." << std::endl;
      return false;
    } else {
      std::cerr << "Error: Unexpected token '" << t.str()
                << "' found on line " << t.line() << "." << std::endl;
      return false;
    }
  }
  return true;
}

bool ArrayExprToken::parse(Tokenizer *tokenizer,
                           std::vector<FunctionToken> &functions,
                           std::vector<GlobalVarToken> &globals) {
  // Make sure the first token is a '{' symbol
  AtomToken first = tokenizer->getNext();
  if (first.empty()) {
    std::cerr << "Error: Unexpected EOF on line " << first.line()
              << "." << std::endl;
    return false;
  } else if ("{" != first.str()) {
    std::cerr << "Error: Unexpected token '" << first.str() << "', expected "
              << "'{' on line " << first.line() << "." << std::endl;
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
      std::cerr << "Error: Unexpected EOF on line " << next.line()
                << "." << std::endl;
      return false;
    } else if ("," != next.str()) {
      std::cerr << "Error: Unexpected token '" << next.str() << "' on line "
                << next.line() << "." << std::endl;
      return false;
    }
  }
  return true;
}

bool GlobalVarToken::parse(Tokenizer *tokenizer,
                           std::vector<FunctionToken> &functions,
                           std::vector<GlobalVarToken> &globals) {
  // Validate the type
  if ("void" == _type.name()) {
    std::cerr << "Error: Global var cannot be of type 'void' on line "
              << _type.line() << "." << std::endl;
    return false;
  }
  // Validate the name
  if (!isValidName(_name)) {
    std::cerr << "Error: Invalid global variable name '" << _name << "' on line "
              << _type.line() << "." << std::endl;
    return false;
  } else if (isFunctionName(_name, functions)) {
    std::cerr << "Error: Global var '" << _name << "' on line " << _type.line()
              << " conflicts with existing function name." << std::endl;
    return false;
  } else if (isGlobalName(_name, globals)) {
    std::cerr << "Error: Global var '" << _name << "' on line " << _type.line()
              << " conflicts with existing global var name." << std::endl;
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
        std::cerr << "Error: Array size mismatch on line " << _type.line()
                  << "." << std::endl;
        return false;
      }
      for (int i = 0; i < arrayExpr.size(); i++) {
        ExprToken expr = arrayExpr.get(i);
        if (!expr.isConst()) {
          std::cerr << "Error: Global value must be known at compile time on line "
                    << expr.line() << "." << std::endl;
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
        std::cerr << "Error: Global value must be known at compile time on line "
                  << expr.line() << "." << std::endl;
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
    std::cerr << "Error: Unexpected EOF on line " << last.line()
              << "." << std::endl;
    return false;
  } else if (";" != last.str()) {
    std::cerr << "Error: Unexpected token '" << last.str() << "', expected ';' "
              << "on line " << last.line() << "." << std::endl;
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
