/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_TOKENIZER_H
#define CONSOLITE_COMPILER_TOKENIZER_H

#include <string>

class Token {
 public:
  Token() : _lineNum(-1) { }
  Token(const std::string& value, int lineNum)
    : _value(value), _lineNum(lineNum) { }
  std::string val() const { return _value; }
  int line() const { return _lineNum; }
 private:
  std::string _value;
  int _lineNum;
};

class Tokenizer {
 public:
  Tokenizer(char *filename);
  ~Tokenizer();
  Token getNext();
  Token peekNext();

 private:
  int _offset;
  int _length;
  int _mmapLength;
  char *_data;
  int _lineNum;
  bool _hasNext;
  Token _next;
};

#endif
