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
  Tokenizer(char *filename);
  ~Tokenizer();
  AtomToken getNext();
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
