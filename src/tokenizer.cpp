/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <fstream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "tokenizer.h"

Tokenizer::Tokenizer(char *filename) : _offset(0), _lineNum(1),
                                       _hasNext(false) {
  int fd = open(filename, O_RDONLY);
  if (-1 == fd) {
    throw "Unable to open input file.";
  }
  _length = lseek(fd, 0, SEEK_END);
  if (-1 == _length) {
    close(fd);
    throw "Unable to get input file length.";
  }
  // Length of memory we allocate with mmap() must be a multiple of
  // the page size.
  _mmapLength = ((_length / getpagesize()) + 1) * getpagesize();
  _data = static_cast<char *>(mmap(NULL, _mmapLength, PROT_READ, MAP_PRIVATE, fd, 0));
  if (MAP_FAILED == static_cast<void *>(_data)) {
    close(fd);
    throw "Unable to map input file.";
  }
  close(fd);
}

Tokenizer::~Tokenizer() {
  munmap(_data, _mmapLength);
}

Token Tokenizer::getNext() {
  // If we have already peeked at this token, we can return
  // it directly.
  if (_hasNext) {
    _hasNext = false;
    return _next;
  }
  std::string token;
  bool singleComment = false;
  bool multiComment = false;
  while (_offset < _length) {
    // In the single line comment state, consume until newline.
    if (singleComment) {
      if ('\n' == _data[_offset]) {
        singleComment = false;
      }
    // In the multi-line comment state, consume until "*/".
    } else if (multiComment) {
      if ('/' == _data[_offset] && 1 <= _offset &&
          '*' == _data[_offset - 1]) {
        multiComment = false;
      }
    // If next char is whitespace, break if we have a partial token,
    // otherwise consume the characters.
    } else if (' ' == _data[_offset] || '\t' == _data[_offset] ||
               '\n' == _data[_offset] || '\r' == _data[_offset]) {
      if (0 < token.length()) {
        break;
      }
    // If next characters start a single line comment, break
    // if we have a partial token, otherwise go into single line
    // comment state.
    } else if ('/' == _data[_offset] && _offset + 1 < _length &&
               '/' == _data[_offset + 1]) {
      if (0 < token.length()) {
        break;
      } else {
        singleComment = true;
      }
    // If next characters start a multi-line comment, break
    // if we have a partial token, otherwise go into multi-line
    // comment state.
    } else if ('/' == _data[_offset] && _offset + 1 < _length &&
               '*' == _data[_offset + 1]) {
      if (0 < token.length()) {
        break;
      } else {
        multiComment = true;
      }
    // If next two characters form a known two-character operator,
    // break if we have a partial token, otherwise set the next
    // two characters as the token and break.
    } else if (_offset + 1 < _length &&
               (('|' == _data[_offset] && '|' == _data[_offset + 1]) ||
                ('&' == _data[_offset] && '&' == _data[_offset + 1]) ||
                ('=' == _data[_offset] && '=' == _data[_offset + 1]) ||
                ('!' == _data[_offset] && '=' == _data[_offset + 1]) ||
                ('<' == _data[_offset] && '=' == _data[_offset + 1]) ||
                ('>' == _data[_offset] && '=' == _data[_offset + 1]))) {
      if (0 == token.length()) {
        token += _data[_offset];
        token += _data[_offset + 1];
        _offset += 2;
      }
      break;
    // If next character forms a known single-character operator,
    // break if we have a partial token, otherwise set the next
    // character as the token and break;
    } else if ('+' == _data[_offset] || '-' == _data[_offset] ||
               '*' == _data[_offset] || '/' == _data[_offset] ||
               '%' == _data[_offset] || '&' == _data[_offset] ||
               '|' == _data[_offset] || '^' == _data[_offset] ||
               '=' == _data[_offset] || '<' == _data[_offset] ||
               '>' == _data[_offset] || '!' == _data[_offset] ||
               '~' == _data[_offset] || ',' == _data[_offset] ||
               ';' == _data[_offset] || '[' == _data[_offset] ||
               ']' == _data[_offset] || '(' == _data[_offset] ||
               ')' == _data[_offset] || '{' == _data[_offset] ||
               '}' == _data[_offset]) {
      if (0 == token.length()) {
        token += _data[_offset];
        _offset++;
      }
      break;
    // If none of the above rules hold, append the next character
    // to the token.
    } else {
      token += _data[_offset];
    }
    if ('\n' == _data[_offset]) {
      _lineNum++;
    }
    _offset++;
  }
  return Token(token, _lineNum);
}

Token Tokenizer::peekNext() {
  _next = getNext();
  _hasNext = true;
  return _next;
}
