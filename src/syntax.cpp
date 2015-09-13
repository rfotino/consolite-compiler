/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include "syntax.h"

GlobalVarToken::GlobalVarToken(const Token& type, const Token& name) {
  _dataType = type.val();
  _varName = name.val();
}

bool GlobalVarToken::parse(Tokenizer *tokenizer,
                           std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  globals = globals;
  return true;
}

FunctionToken::FunctionToken(const Token& type, const Token& name) {
  _returnType = type.val();
  _funcName = name.val();
}

bool FunctionToken::parse(Tokenizer *tokenizer,
                          std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  globals = globals;
  return true;
}
