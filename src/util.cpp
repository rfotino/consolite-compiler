/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <regex>
#include "util.h"

/**
 * Puts this operand's value in the given register. Only requires
 * the use of the one register.
 */
void operandValueToReg(Parser *parser,
                       const Operand& operand,
                       const std::string& reg) {
  if (OperandType::ADDRESS == operand.type()) {
    parser->writeInst("POP " + reg);
    parser->writeInst("LOAD " + reg + " " + reg);
  } else if (OperandType::REGISTER == operand.type()) {
    parser->writeInst("MOV " + reg + " " + operand.reg());
  } else if (OperandType::VALUE == operand.type()) {
    parser->writeInst("POP " + reg);
  } else if (OperandType::LITERAL == operand.type()) {
    parser->writeInst("MOVI " + reg + " " + toHexStr(operand.literal()));
  }
}

/**
 * Returns a hex string of the form "0x0000" for the
 * given value, where digits is the number of digits after
 * the "0x".
 */
std::string toHexStr(uint16_t value, int digits) {
  std::string str;
  for (int i = 0; i < digits; i++) {
    uint8_t next = value & 0xf;
    value >>= 4;
    str = (char)(next <= 9 ? '0' + next : 'a' - 10 + next) + str;
  }
  return "0x" + str;
}

/**
 * Returns the opposing paranthesis for (), [], or {} pairs.
 * Returns an empty string if the input is not one of the above.
 */
std::string otherParen(const std::string& paren) {
  if ("(" == paren) {
    return ")";
  } else if (")" == paren) {
    return "(";
  } else if ("[" == paren) {
    return "]";
  } else if ("]" == paren) {
    return "[";
  } else if ("{" == paren) {
    return "}";
  } else if ("}" == paren) {
    return "{";
  }
  return "";
}

/**
 * Returns true if the given string is a valid name for a function,
 * variable, etc. A valid name starts with an alphabetic or underscore
 * character, and is followed by zero or more alphanumeric or
 * underscore characters.
 */
bool isValidName(const std::string& name) {
  return std::regex_match(name, std::regex("^[_a-zA-Z][_a-zA-Z0-9]*$"));
}

/**
 * Returns true if the given string is a valid label declaration.
 * A label declaration starts with a valid name and is followed by
 * a colon.
 */
bool isLabelDeclaration(const std::string& label) {
  return std::regex_match(label, std::regex("^[_a-zA-Z][_a-zA-Z0-9]*:$"));
}

/**
 * Returns true if the given funcName is a builtin function.
 */
bool isBuiltin(const std::string& funcName) {
  static std::vector<std::string> builtins = { "COLOR", "PIXEL", "TIMERST",
                                               "TIME", "INPUT", "RND" };
  return std::find(builtins.begin(), builtins.end(), funcName) != builtins.end();
}

/**
 * Searches through a vector of parameters and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<ParamToken> getParameter(
      const std::string& name,
      const std::vector<std::shared_ptr<ParamToken>>& parameters) {
  for (auto param : parameters) {
    if (param->name() == name) {
      return param;
    }
  }
  return nullptr;
}

/**
 * Searches through a vector of local variables and returns the one that
 * matches the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<LocalVarToken> getLocal(
      const std::string& name,
      const std::vector<std::shared_ptr<LocalVarToken>>& locals) {
  for (auto local : locals) {
    if (local->name() == name) {
      return local;
    }
  }
  return nullptr;
}

/**
 * Searches through a vector of functions and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<FunctionToken> getFunction(
      const std::string& name,
      const std::vector<std::shared_ptr<FunctionToken>>& functions) {
  for (auto func : functions) {
    if (func->name() == name) {
      return func;
    }
  }
  return nullptr;
}

/**
 * Searches through a vector of globals and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<GlobalVarToken> getGlobal(
     const std::string& name,
     const std::vector<std::shared_ptr<GlobalVarToken>>& globals) {
  for (auto global : globals) {
    if (global->name() == name) {
      return global;
    }
  }
  return nullptr;
}

/**
 * Searches through a vector of labels and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<LabelStatement> getLabel(
      const std::string& name,
      const std::vector<std::shared_ptr<LabelStatement>>& labels) {
  for (auto label : labels) {
    if (label->name() == name) {
      return label;
    }
  }
  return nullptr;
}

/**
 * Returns true if the given string names a valid type. There
 * are only a few valid types right now so this function
 * is a bit crude.
 */
bool isType(const std::string& type) {
  return "void" == type ||  "uint16" == type;
}

/**
 * Prints an error message with the given line number.
 */
void _error(const std::string& msg, int lineNum) {
  // TODO: It would be nice to have error messages that showed you
  // the source line and pointed to the token causing an error.
  std::cerr << "Error:";
  if (0 < lineNum) {
    std::cerr << lineNum << ":";
  }
  std::cerr << " " << msg << std::endl;
}

/**
 * Prints a warning message with the given line number.
 */
void _warn(const std::string& msg, int lineNum) {
  std::cerr << "Warning:";
  if (0 < lineNum) {
    std::cerr << lineNum << ":";
  }
  std::cerr << " " << msg << std::endl;
}

/**
 * Consumes the next token, and prints an error message and returns
 * false if it finds EOF or a token other than the one it was
 * expecting. Returns true if the next token was the expected token.
 */
bool _expect(Tokenizer *tokenizer, const std::string& str, bool errors) {
  AtomToken t = tokenizer->getNext();
  if (t.str().empty()) {
    if (errors) {
      _error("Unexpected EOF, expected '" + str + "'.", t.line());
    }
    return false;
  } else if (str != t.str()) {
    if (errors) {
      _error("Unexpected token '" + t.str() + "', expected '" + str + "'.",
             t.line());
    }
    return false;
  }
  return true;
}
