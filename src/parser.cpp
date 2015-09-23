/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include "parser.h"
#include "util.h"

Parser::Parser(Tokenizer *t) : _tokenizer(t), _bytePos(0) {
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
      _error("Error: Unexpected EOF, expected global or function name.");
      return false;
    }

    // Differentiate between function and global variable
    if ("(" == _tokenizer->peekNext().str()) {
      std::shared_ptr<FunctionToken> func(new FunctionToken(type, name.str()));
      if (!func->parse(_tokenizer, _functions, _globals)) {
        return false;
      }
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
    _error("No 'void main()' entry point found.");
    return false;
  }
  return true;
}

bool Parser::output(char *filename) {
  // Start by opening the output file.
  _outfile.open(filename, std::ofstream::out | std::ofstream::trunc);
  if (!_outfile.good()) {
    _error("Unable to open output file.");
    return false;
  }
  // Next assign labels for all globals and functions.
  for (auto global : _globals) {
    this->addLabel(global->name());
  }
  for (auto function : _functions) {
    this->addLabel(function->name());
  }
  // Output the "bootloader".
  this->writeInst("JMPI main");
  // Output global variables.
  for (auto global : _globals) {
    global->output(this);
  }
  // Output functions.
  for (auto function: _functions) {
    function->output(this);
  }
  return true;
}

bool Parser::hasLabel(const std::string& label) {
  return 0 < _assignedLabels.count(label);
}

bool Parser::addLabel(const std::string& label) {
  if (this->hasLabel(label)) {
    return false;
  }
  _assignedLabels.insert(label);
  return true;
}

std::string Parser::getUnusedLabel(const std::string& label) {
  if (!this->hasLabel(label)) {
    this->addLabel(label);
    return label;
  }
  int i = 1;
  while (this->hasLabel(label + std::to_string(i))) {
    i++;
  }
  this->addLabel(label + std::to_string(i));
  return label + std::to_string(i);
}

void Parser::writeInst(const std::string& inst) {
  this->writeln("        " + inst);
  _bytePos += INST_SIZE;
}

void Parser::writeData(const std::string& data, int dataLength) {
  this->writeln("        " + data);
  _bytePos += dataLength * DATA_SIZE;
  while (0 != _bytePos % INST_SIZE) {
    _bytePos++;
  }
}

void Parser::writeln(const std::string& line) {
  std::cout << line << std::endl;
  _outfile << line << std::endl;
}
