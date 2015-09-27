/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_UTIL_H
#define CONSOLITE_COMPILER_UTIL_H

#include <iostream>
#include "tokenizer.h"
#include "syntax.h"
#include "parser.h"

#define ADDRESS_SIZE 2
#define DATA_SIZE    2
#define INST_SIZE    4

/**
 * Puts this operand's value in the given register. Only requires
 * the use of the one register.
 */
void operandValueToReg(Parser *parser,
                       const Operand& operand,
                       const std::string& reg);
/**
 * Returns a hex string of the form "0x0000" for the
 * given value, where digits is the number of digits after
 * the "0x".
 */
std::string toHexStr(uint16_t value, int digits = 4);

/**
 * Returns the opposing paranthesis for (), [], or {} pairs.
 * Returns an empty string if the input is not one of the above.
 */
std::string otherParen(const std::string& paren);

/**
 * Returns true if the given string is a valid name for a function,
 * variable, etc. A valid name starts with an alphabetic or underscore
 * character, and is followed by zero or more alphanumeric or
 * underscore characters.
 */
bool isValidName(const std::string& name);

/**
 * Returns true if the given string is a valid label declaration.
 * A label declaration starts with a valid name and is followed by
 * a colon.
 */
bool isLabelDeclaration(const std::string& label);

/**
 * Returns true if the given funcName is a builtin function.
 */
bool isBuiltin(const std::string& funcName);

/**
 * Searches through a vector of parameters and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<ParamToken> getParameter(
      const std::string& name,
      const std::vector<std::shared_ptr<ParamToken>>& parameters);

/**
 * Searches through a vector of local variables and returns the one that
 * matches the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<LocalVarToken> getLocal(
      const std::string& name,
      const std::vector<std::shared_ptr<LocalVarToken>>& locals);

/**
 * Searches through a vector of functions and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<FunctionToken> getFunction(
      const std::string& name,
      const std::vector<std::shared_ptr<FunctionToken>>& functions);

/**
 * Searches through a vector of globals and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<GlobalVarToken> getGlobal(
     const std::string& name,
     const std::vector<std::shared_ptr<GlobalVarToken>>& globals);

/**
 * Searches through a vector of labels and returns the one that matches
 * the given name, or a null pointer if the name was not found.
 */
std::shared_ptr<LabelStatement> getLabel(
      const std::string& name,
      const std::vector<std::shared_ptr<LabelStatement>>& labels);

/**
 * Returns true if the given string names a valid type. There
 * are only a few valid types right now so this function
 * is a bit crude.
 */
bool isType(const std::string& type);

/**
 * Prints an error message with the given line number.
 */
void _error(const std::string& msg, int lineNum = -1);

/**
 * Prints a warning message with the given line number.
 */
void _warn(const std::string& msg, int lineNum = -1);

/**
 * Consumes the next token, and prints an error message and returns
 * false if it finds EOF or a token other than the one it was
 * expecting. Returns true if the next token was the expected token.
 */
bool _expect(Tokenizer *tokenizer, const std::string& str, bool errors = true);

#endif
