/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include "parser.h"

bool Parser::parse() {
  bool error = false;
  while (true) {
    // Get the type
    if (_tokenizer->peekNext().empty()) {
      // We have reached the end of the token stream without errors
      break;
    }
    TypeToken type;
    if (!type.parse(_tokenizer, _functions, _globals)) {
      error = true;
      break;
    }

    // Get the name
    AtomToken name = _tokenizer->getNext();
    if (name.empty()) {
      error = true;
      std::cerr << "Error: Unexpected EOF, expected global or function name."
                << std::endl;
      break;
    }

    // Differentiate between function and global variable
    if ("(" == _tokenizer->peekNext().str()) {
      std::shared_ptr<FunctionToken> func(new FunctionToken(type, name.str()));
      if (!func->parse(_tokenizer, _functions, _globals)) {
        error = true;
        break;
      }
      _functions.push_back(func);
    } else {
      std::shared_ptr<GlobalVarToken> var(new GlobalVarToken(type, name.str()));
      if (!var->parse(_tokenizer, _functions, _globals)) {
        error = true;
        break;
      }
      _globals.push_back(var);
    }
  }
  return !error;
}

void Parser::output(char *filename) {
  std::cerr << "Error: Parser::output() not yet implemented. Output file '"
            << filename << "' is unchanged." << std::endl;
}
