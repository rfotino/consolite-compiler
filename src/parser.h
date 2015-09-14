/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_PARSER_H
#define CONSOLITE_COMPILER_PARSER_H

#include <vector>
#include "tokenizer.h"
#include "syntax.h"

class Parser {
 public:
  Parser(Tokenizer *t) : _tokenizer(t) { }
  bool parse();
  void output(char *filename);

 private:
  Tokenizer *_tokenizer;
  std::vector<GlobalVarToken> _globals;
  std::vector<FunctionToken> _functions;
};

#endif
