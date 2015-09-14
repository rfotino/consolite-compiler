/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include "parser.h"

bool Parser::parse() {
  bool error = false;
  while (true) {
    if (_tokenizer->peekNext().val().empty()) {
      // We have reached the end of the token stream without errors
      break;
    }
    TypeToken type;
    if (!type.parse(_tokenizer, _functions, _globals)) {
      error = true;
      break;
    }

    if (_tokenizer->peekNext().val().empty()) {
      error = true;
      std::cerr << "Error: Unexpected EOF, expected global or function name."
                << std::endl;
      break;
    }
    NameToken name;
    if (!name.parse(_tokenizer, _functions, _globals)) {
      error = true;
      break;
    }

    if ("(" == _tokenizer->peekNext().val()) {
      std::shared_ptr<FunctionToken> func(new FunctionToken(type, name));
      if (!func->parse(_tokenizer, _functions, _globals)) {
        error = true;
        break;
      }
      _functions.push_back(func);
    } else {
      std::shared_ptr<GlobalVarToken> var(new GlobalVarToken(type, name));
      if (!var->parse(_tokenizer, _globals)) {
        error = true;
        break;
      }
      _globals.push_back(var);
    }
  }
  return error;
}

void Parser::output(char *filename) {
  filename = filename;
}
