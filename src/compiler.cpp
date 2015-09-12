/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>
#include "tokenizer.h"

void usage(char *program_name) {
  std::cout << "Usage: " << program_name << " SRC DEST" << std::endl;
}

int main(int argc, char **argv) {
  if (3 != argc) {
    usage(argv[0]);
    return 1;
  }

  Tokenizer tokenizer(argv[1]);
  if (tokenizer.hasError()) {
    std::cout << tokenizer.getError() << std::endl;
    return 1;
  }

  std::string token;
  while ("" != (token = tokenizer.getNext())) {
    std::cout << token << std::endl;
  }

  return 0;
}
