/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include "tokenizer.h"

Tokenizer::Tokenizer(char *filename) : _offset(0), _lineNum(1),
                                       _hasNext(false) {
  std::ifstream inputStream(filename);
  inputStream.seekg(0, std::ios::end);
  _data.reserve(inputStream.tellg());
  inputStream.seekg(0, std::ios::beg);
  _data.assign(std::istreambuf_iterator<char>(inputStream),
               std::istreambuf_iterator<char>());
}

AtomToken Tokenizer::getNext() {
  // If we have already peeked at this token, we can return
  // it directly.
  if (_hasNext) {
    _hasNext = false;
    return _next;
  }
  std::string token;
  bool singleComment = false;
  bool multiComment = false;
  while (_offset < _data.length()) {
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
    } else if ('/' == _data[_offset] && _offset + 1 < _data.length() &&
               '/' == _data[_offset + 1]) {
      if (0 < token.length()) {
        break;
      } else {
        singleComment = true;
      }
    // If next characters start a multi-line comment, break
    // if we have a partial token, otherwise go into multi-line
    // comment state.
    } else if ('/' == _data[_offset] && _offset + 1 < _data.length() &&
               '*' == _data[_offset + 1]) {
      if (0 < token.length()) {
        break;
      } else {
        multiComment = true;
      }
    // If next two characters form a known two-character operator,
    // break if we have a partial token, otherwise set the next
    // two characters as the token and break.
    } else if (_offset + 1 < _data.length() &&
               (('|' == _data[_offset] && '|' == _data[_offset + 1]) ||
                ('&' == _data[_offset] && '&' == _data[_offset + 1]) ||
                ('=' == _data[_offset] && '=' == _data[_offset + 1]) ||
                ('!' == _data[_offset] && '=' == _data[_offset + 1]) ||
                ('<' == _data[_offset] && '=' == _data[_offset + 1]) ||
                ('>' == _data[_offset] && '=' == _data[_offset + 1]) ||
                ('<' == _data[_offset] && '<' == _data[_offset + 1]) ||
                ('>' == _data[_offset] && '>' == _data[_offset + 1]))) {
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
  return AtomToken(token, _lineNum);
}

AtomToken Tokenizer::peekNext() {
  _next = getNext();
  _hasNext = true;
  return _next;
}
