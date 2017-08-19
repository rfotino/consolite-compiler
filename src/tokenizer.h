/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_TOKENIZER_H
#define CONSOLITE_COMPILER_TOKENIZER_H

#include <string>
#include "syntax.h"

class Tokenizer {
 public:
  /**
   * Opens the given file and mmaps it to an internal pointer for easy
   * traversal. Throws an exception on failure.
   */
  Tokenizer(char *filename);

  /**
   * Consumes the next token and returns it.
   */

  AtomToken getNext();
  /**
   * Returns the next token without consuming it. Subsequent calls to
   * getNext() or peekNext() will return the same token.
   */
  AtomToken peekNext();

 private:
  unsigned int _offset;
  unsigned int _lineNum;
  bool _hasNext;
  AtomToken _next;
  std::string _data;
};

#endif
