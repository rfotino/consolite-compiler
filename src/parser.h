/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_PARSER_H
#define CONSOLITE_COMPILER_PARSER_H

#include <memory>
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
  std::vector<std::shared_ptr<GlobalVarToken>> _globals;
  std::vector<std::shared_ptr<FunctionToken>> _functions;
};

#endif
