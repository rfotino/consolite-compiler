/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#include <iostream>

void usage(char *program_name) {
  std::cout << "Usage: " << program_name << " SRC DEST" << std::endl;
}

int main(int argc, char **argv) {
  if (3 != argc) {
    usage(argv[0]);
    return 1;
  }

  std::cout << "The compiler is not implemented yet, come back later!"
            << std::endl;

  return 0;
}
