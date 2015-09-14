/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include "syntax.h"

bool isType(std::string type) {
  return "int16" == type || "uint16" == type;
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

bool NameToken::parse(Tokenizer *tokenizer,
                      std::vector<std::shared_ptr<FunctionToken>> &functions,
                      std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  functions = functions;
  globals = globals;
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

bool GlobalVarToken::parse(Tokenizer *tokenizer,
                           std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  globals = globals;
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
