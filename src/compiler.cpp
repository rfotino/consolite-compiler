/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include "tokenizer.h"
#include "parser.h"

void usage(char *program_name) {
  std::cout << "Usage: " << program_name << " SRC DEST" << std::endl;
}

int main(int argc, char **argv) {
  if (3 != argc) {
    usage(argv[0]);
    return 1;
  }

  try {
    // Creates a stream of tokens from the input file
    Tokenizer tokenizer(argv[1]);
    // Parses the tokens into an abstract syntax tree
    Parser parser(&tokenizer);
    parser.parse();
    // Compiles the syntax tree to the output file
    parser.output(argv[2]);
  } catch (char const *error) {
    std::cout << error << std::endl;
    return 1;
  }

  return 0;
}
