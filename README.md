# Consolite Compiler

This program compiles a C-like higher level language to
[Consolite Assembly](https://github.com/rfotino/consolite-assembler/blob/master/docs/assembly.md).
You can the current draft of the language specification [here](docs/spec.md).

## What Is Consolite?

Consolite (coming from "Console Lite") is the name I've given to my design of a
hobbyist microprocessor and associated toolchain. Consolite's instruction set
architecture (ISA) has assembly level instructions for setting colors, drawing
pixels on the screen, and receiving input. This makes it suitable for writing
games, which is what makes it a "console".

### Consolite Specs

* Display: 256 x 192 pixels, 8-bit color
* Registers: 16 general purpose, instruction pointer, color, flags
* Memory: 64KiB main memory, 48KiB video memory

## Building and Dependencies

This version of the compiler is written in C++, and uses only the standard
library. To build the project just type `make`.

## Usage

`./compiler SRC DEST`

## Copyright

Copyright (c) 2015 Robert Fotino, All Rights Reserved
