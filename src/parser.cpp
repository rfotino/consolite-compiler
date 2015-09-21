/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include "parser.h"
#include "util.h"

Parser::Parser(Tokenizer *t) : _tokenizer(t) {
  // Add builtin "void COLOR(uint16 color)" function.
  _functions.push_back(
    std::make_shared<FunctionToken>(
      FunctionToken(
        TypeToken("void"),
        "COLOR",
        {
          std::make_shared<ParamToken>(
            ParamToken(TypeToken("uint16"), "color")
          )
        }
      )
    )
  );
  // Add builtin "void PIXEL(uint16 x, uint16 y)" function.
  _functions.push_back(
    std::make_shared<FunctionToken>(
      FunctionToken(
        TypeToken("void"),
        "PIXEL",
        {
          std::make_shared<ParamToken>(
            ParamToken(TypeToken("uint16"), "x")
          ),
          std::make_shared<ParamToken>(
            ParamToken(TypeToken("uint16"), "y")
          )
        }
      )
    )
  );
  // Add builtin "void TIMERST()" function.
  _functions.push_back(
    std::make_shared<FunctionToken>(
      FunctionToken(
        TypeToken("void"),
        "TIMERST",
        { }
      )
    )
  );
  // Add builtin "uint16 TIME()" function.
  _functions.push_back(
    std::make_shared<FunctionToken>(
      FunctionToken(
        TypeToken("uint16"),
        "TIME",
        { }
      )
    )
  );
  // Add builtin "uint16 INPUT(uint16 input_id)" function.
  _functions.push_back(
    std::make_shared<FunctionToken>(
      FunctionToken(
        TypeToken("uint16"),
        "INPUT",
        {
          std::make_shared<ParamToken>(
            ParamToken(TypeToken("uint16"), "input_id")
          )
        }
      )
    )
  );
}

bool Parser::parse() {
  while (true) {
    // Get the type
    if (_tokenizer->peekNext().empty()) {
      // We have reached the end of the token stream without errors
      break;
    }
    TypeToken type;
    if (!type.parse(_tokenizer, _functions, _globals)) {
      return false;
    }

    // Get the name
    AtomToken name = _tokenizer->getNext();
    if (name.empty()) {
      std::cerr << "Error: Unexpected EOF, expected global or function name."
                << std::endl;
      return false;
    }

    // Differentiate between function and global variable
    if ("(" == _tokenizer->peekNext().str()) {
      std::shared_ptr<FunctionToken> func(new FunctionToken(type, name.str()));
      if (!func->parse(_tokenizer, _functions, _globals)) {
        return false;
      }
      _functions.push_back(func);
    } else {
      std::shared_ptr<GlobalVarToken> var(new GlobalVarToken(type, name.str()));
      if (!var->parse(_tokenizer, _functions, _globals)) {
        return false;
      }
      _globals.push_back(var);
    }
  }
  // Make sure there is a 'void main()' function, which is the entry point.
  auto entryPoint = getFunction("main", _functions);
  if (!entryPoint ||
      "void" != entryPoint->type().name() ||
      0 != entryPoint->numParams()) {
    std::cerr << "Error: No 'void main()' entry point found." << std::endl;
    return false;
  }
  return true;
}

void Parser::output(char *filename) {
  std::cerr << "Error: Parser::output() not yet implemented. Output file '"
            << filename << "' is unchanged." << std::endl;
}
