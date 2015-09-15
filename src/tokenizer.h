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
  ~Tokenizer();
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
  int _offset;
  int _length;
  int _mmapLength;
  char *_data;
  int _lineNum;
  bool _hasNext;
  AtomToken _next;
};

#endif
