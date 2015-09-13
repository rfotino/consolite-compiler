/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_TOKENIZER_H
#define CONSOLITE_COMPILER_TOKENIZER_H

#include <string>

class Tokenizer {
 public:
  Tokenizer(char *filename);
  ~Tokenizer();
  std::string getNext();
  std::string peekNext();
  int getLineNum() { return _lineNum; }

 private:
  int _offset;
  int _length;
  int _mmapLength;
  char *_data;
  int _lineNum;
};

#endif
