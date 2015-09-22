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
   * Writes a line of output to the outfile.
   */
  void writeln(const std::string& line);

 private:
  Tokenizer *_tokenizer;
  std::vector<std::shared_ptr<GlobalVarToken>> _globals;
  std::vector<std::shared_ptr<FunctionToken>> _functions;
  std::ofstream _outfile;
  /**
   * A list of assembly-level labels that have already been used.
   * These are saved so that we don't have conflicting label names.
   */
  std::unordered_set<std::string> _assignedLabels;
};

#endif
