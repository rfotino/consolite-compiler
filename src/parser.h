/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_PARSER_H
#define CONSOLITE_COMPILER_PARSER_H

#include <vector>
#include <fstream>
#include <unordered_set>
#include "tokenizer.h"

class Parser {
 public:
  Parser(Tokenizer *t);
  /**
   * Parses the tokens from the Tokenizer into an abstract syntax tree.
   * Returns false if there were errors found in the source code.
   */
  bool parse();
  /**
   * Converts the abstract syntax tree to assembly and outputs it to
   * the given file. Returns false if it encounters an error.
   */
  bool output(char *filename);
  /**
   * Tests if an assembly-level label has already been used.
   */
  bool hasLabel(const std::string& label);
  /**
   * Tries to add the given assembly-level label to the list of used
   * labels. If the label has already been used, return false and do nothing.
   */
  bool addLabel(const std::string& label);
  /**
   * Returns a valid, unused label that includes the given base label.
   */
  std::string getUnusedLabel(const std::string& base);
  /**
   * Writes an instruction to the outfile and increases the byte count of
   * the output by the instruction length.
   */
  void writeInst(const std::string& inst);
  /**
   * Writes some data to the outfile and increases the byte count of the
   * output.
   */
  void writeData(const std::string& data, int dataLength);
  /**
   * Writes a line of output to the outfile.
   */
  void writeln(const std::string& line);
  /**
   * Gets the current byte position of the output.
   */
  uint16_t getBytePos() const { return _bytePos; }

 private:
  Tokenizer *_tokenizer;
  std::vector<std::shared_ptr<GlobalVarToken>> _globals;
  std::vector<std::shared_ptr<FunctionToken>> _functions;
  std::ofstream _outfile;
  /**
   * The register that was most recently requested to be PUSHed onto
   * the stack. Used for optimizing the PUSH followed by POP pattern.
   */
  std::string _pendingPushReg;
  /**
   * A list of assembly-level labels that have already been used.
   * These are saved so that we don't have conflicting label names.
   */
  std::unordered_set<std::string> _assignedLabels;
  /**
   * The current byte count of the output. Used to know the current
   * address.
   */
  uint16_t _bytePos;
};

#endif
