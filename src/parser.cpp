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
  // Add builtin "uint16 RND()" function.
  _functions.push_back(
    std::make_shared<FunctionToken>(
      FunctionToken(
        TypeToken("uint16"),
        "RND",
        { }
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
  // Output the "bootloader". This sets the stack pointer, calls main,
  // then goes into an infinite loop to prevent attempting to execute
  // code that wasn't meant to be executed.
  std::string stackLabel = this->getUnusedLabel("stack");
  this->writeInst("MOVI SP " + stackLabel);
  this->writeInst("CALL main");
  std::string finishedLabel = this->getUnusedLabel("program_finished");
  this->writeln(finishedLabel + ":");
  this->writeInst("JMPI " + finishedLabel);
  // Output global variables.
  for (auto global : _globals) {
    global->output(this);
  }
  // Output functions.
  for (auto function: _functions) {
    function->output(this);
  }
  // Output the stack position.
  this->writeln(stackLabel + ":");
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
  // We want to optimize a PUSH followed by a POP by either
  // removing it entirely or by turning it into a single MOV
  // instruction.
  if (_pendingPushReg.empty() &&
      0 == inst.compare(0, std::string("PUSH").size(), "PUSH")) {
    // It is a PUSH instruction and there are no pending push instructions.
    // We need to save the register and write nothing, because the next
    // instruction could be a POP.
    _pendingPushReg = inst.substr(std::string("PUSH ").size());
    return;
  } else if (!_pendingPushReg.empty() &&
             0 == inst.compare(0, std::string("POP").size(), "POP")) {
    // It is a POP instruction and the previous instruction was a PUSH.
    // We can optimize this into nothing or a MOV instruction.
    std::string popReg = inst.substr(std::string("POP ").size());
    std::string pushReg = _pendingPushReg;
    _pendingPushReg = "";
    if (popReg != pushReg) {
      this->writeInst("MOV " + popReg + " " + pushReg);
    }
    return;
  }
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
  // If there is a pending PUSH instruction, first write it
  // to the outfile.
  if (!_pendingPushReg.empty()) {
    _outfile << "        PUSH " << _pendingPushReg << std::endl;
    _pendingPushReg = "";
  }
  // Then write the new line.
  _outfile << line << std::endl;
}
