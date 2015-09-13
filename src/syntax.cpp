/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include "syntax.h"

bool GlobalVarToken::parse(Tokenizer *tokenizer,
                           std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  globals = globals;
  return true;
}

bool FunctionToken::parse(Tokenizer *tokenizer,
                          std::vector<std::shared_ptr<GlobalVarToken>> &globals) {
  tokenizer = tokenizer;
  globals = globals;
  return true;
}
