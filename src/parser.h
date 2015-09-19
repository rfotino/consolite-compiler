/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_PARSER_H
#define CONSOLITE_COMPILER_PARSER_H

#include <vector>
#include "tokenizer.h"

class Parser {
 public:
  Parser(Tokenizer *t) : _tokenizer(t) { }
  /**
   * Parses the tokens from the Tokenizer into an abstract syntax tree.
   * Returns false if there were errors found in the source code.
   */
  bool parse();
  /**
   * Converts the abstract syntax tree to assembly and outputs it to
   * the given file. Throws an exception on error.
   */
  void output(char *filename);

 private:
  Tokenizer *_tokenizer;
  std::vector<std::shared_ptr<GlobalVarToken>> _globals;
  std::vector<std::shared_ptr<FunctionToken>> _functions;
};

#endif
