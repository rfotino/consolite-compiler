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
  bool hasError() { return _error; }
  std::string getError() { return _errorMsg; }

 private:
  int _offset;
  int _length;
  int _mmapLength;
  char *_data;
  bool _error;
  std::string _errorMsg;
};

#endif