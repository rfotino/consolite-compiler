/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include <regex>
#include "syntax.h"

bool isValidName(const std::string& name) {
  return std::regex_match(name, std::regex("^[_a-zA-Z][_a-zA-Z0-9]*$"));
}

bool isFunctionName(const std::string& name,
                    const std::vector<std::shared_ptr<FunctionToken>>& functions) {
  for (auto func : functions) {
    if (func->name() == name) {
      return true;
    }
  }
  return false;
}

bool isGlobalName(const std::string& name,
                  const std::vector<std::shared_ptr<GlobalVarToken>>& globals) {
  for (auto global : globals) {
    if (global->name() == name) {
      return true;
    }
  }
  return false;
}

bool isType(const std::string& type) {
  return "void" == type || "int16" == type || "uint16" == type;
}

bool TypeToken::parse(Tokenizer *tokenizer,
                      std::vector<std::shared_ptr<FunctionToken>> &functions,
                      std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  Token typeName = tokenizer->getNext();
  if (!isType(typeName.val())) {
    std::cerr << "Error: Invalid type '" << typeName.val() << "' on line "
              << typeName.line() << "." << std::endl;
    return false;
  }
  _lineNum = typeName.line();
  _name = typeName.val();
  if ("[" == tokenizer->peekNext().val()) {
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
    _arraySize = expr.constVal(globals);
    Token endBracket = tokenizer->getNext();
    if ("]" != endBracket.val()) {
      std::cerr << "Error: Unexpected token '" << endBracket.val()
                << "', expected ']' on line " << endBracket.line()
                << "." << std::endl;
      return false;
    }
  }
  return true;
}

bool ExprToken::parse(Tokenizer *tokenizer,
                      std::vector<std::shared_ptr<FunctionToken>> &functions,
                      std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  functions = functions;
  globals = globals;
  return true;
}

int ExprToken::constVal(std::vector<std::shared_ptr<GlobalVarToken>> &globals) const {
  if (!_const) {
    return 0;
  }
  globals = globals;
  return 0;
}

bool ArrayExprToken::parse(Tokenizer *tokenizer,
                           std::vector<std::shared_ptr<FunctionToken>> &functions,
                           std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  functions = functions;
  globals = globals;
  return true;
}

bool GlobalVarToken::parse(Tokenizer *tokenizer,
                           std::vector<std::shared_ptr<FunctionToken>> &functions,
                           std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
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
  Token next = tokenizer->getNext();
  Token last = next;
  if ("=" == next.val()) {
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
        _arrayValues.push_back(expr.constVal(globals));
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
      _value = expr.constVal(globals);
    }
    // This should be a semicolon
    last = tokenizer->getNext();
  } else if (";" == next.val()) {
    // No value supplied, give default value of 0.
    if (_type.isArray()) {
      for (int i = 0; i < _type.arraySize(); i++) {
        _arrayValues.push_back(0);
      }
    } else {
      _value = 0;
    }
  }
  if (last.val().empty()) {
    std::cerr << "Error: Unexpected EOF on line " << last.line()
              << "." << std::endl;
    return false;
  } else if (";" != last.val()) {
    std::cerr << "Error: Unexpected token '" << last.val() << "', expected ';' "
              << "on line " << last.line() << "." << std::endl;
    return false;
  }
  return true;
}

bool FunctionToken::parse(Tokenizer *tokenizer,
                          std::vector<std::shared_ptr<FunctionToken>> &functions,
                          std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  functions = functions;
  globals = globals;
  return true;
}
