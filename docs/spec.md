# Consolite Higher Level Language Specification

This document specifies a C-like higher level language that compiles to [Consolite Assembly](https://github.com/rfotino/consolite-assembler/blob/master\
/docs/assembly.md). It has support for functions, loops, if-else, variables, arrays, arithmetic expressions, etc, as well as builtin functions for accessing special assembly instructions like PIXEL, COLOR, TIME, TIMERST, and INPUT. There is no name for this higher level language as of yet, so I will simply call it "Consolite C" for the remainder of this document.

This document is a work in progress, and it may be incomplete or contain ambiguities. I will try to iron these issues out as the design of the language matures.

## Comments

Comment syntax is the same as in C, single line comments start with `//` while multi-line comments begin with `/*` and end with `*/`. These comments are stripped out before compilation. For example:

```c
// I'm a single line comment, I don't affect the code!
/* I'm a multi-line comment,
   I don't affect the code either! */
```

## Data Types

### Primitives

Consolite C currently has only one primitive data type, though I hope to expand on that eventually by adding signed 16-bit values, signed/unsigned 8-bit values, and floating point values.

| Name   | Description                 | Size    | Value Range |
|--------|-----------------------------|---------|-------------|
| uint16 | An unsigned 16-bit integer. | 2 bytes | [0, 65535]  |

### Arrays

Arrays in Consolite C must have a constant size that can be determined at compile time. The size of an array can contain arithmetic with global variables, and the compiler will use the *initial value* of those global variables to determine the array size. I hope to expand on this by adding a `const` keyword eventually. Arrays cannot be multi-dimensional. For example:

```c
// Declares an array of initialized int16 values.
int16[5] arr1 = { -2, -1, 0, 1, 2 };
// Declares an array of uninitialized uint16 values.
uint16[4] arr2;
```

### Pointers

In Consolite C there are currently no pointer types. If you need to store an address, you should use a `uint16` type.

## Variables

### Valid Names

A valid variable name starts with an underscore or an alphabetic character, and is followed by zero or more underscore or alphanumeric characters. A variable name cannot be the same as a variable already declared and accessible from the current scope, and it cannot be the same as a reserved word.

### Scope

Variables have either global scope or they have scope local to a function. Loops and code blocks surrounded with `{ }` do not have their own scope, but have the same scope as the containing function.

### Global Variables

Global variables are declared outside of a function, and if they have an initial value it must be known at compile time. This means the initialization expression cannot use any dereferencing operators, address-of operators, assignment operators, function calls. Uninitialized global variables will have an initial value of zero.

### Local Variables

Local variables must be declared at the top of a function, before any other statements. They can include an initial value, which does not need to be known at compile-time. The values of uninitialized local variables are indeterminate, since they depend on what was on the stack leading up to the allocation of storage for the local variable.

## Control Flow

### If-Else Statements

Syntax (the `else` part of the statement can be omitted):

```c
if ([condition])
  [true-statement]
else
  [false-statement]
```

Example:

```c
uint16 a = 0, b = 1;
if (a < b) {
  // Do something here
} else {
  // Do something else here
}
```

If the `[condition]` evaluates to a value other than zero, then the `[true-statement]` will be executed. Otherwise the `[false-statement]` will be executed.

Because `[false-statement]` can also be an if-else statement, you can chain these into something like the following:

```c
if ([condition1])
  [statement1]
else if ([condition2])
  [statement2]
else
  [statement3]
```

### Labels and goto

Example:

```c
uint16 i = 0;
label:
i = i + 1;
if (i < 10) {
  goto label;
}
```

Labels are declared with a name followed by a colon. They are local to a function, meaning you can't use goto to jump between functions and you can have labels with the same name in different functions.

The `goto` statement does an unconditional jump to a specified label within the current scope.

### For Loops

Syntax:

```c
for ([initialization]; [condition]; [final-statement])
  [statement]
```

Equivalent to:

```c
[initialization]
start:
if (![condition])
  goto end;
[statement]
[final-statement]
goto start;
end:
```

Example:
```c
uint16 i;
for (i = 0; i < 32; i = i + 1) {
  // Loop body, do something here
}
```

For loops are often used to iterate over an array, or to do an action a set number of times.

### While Loops

Syntax:

```c
while ([condition])
  [statement]
```

Equivalent to:

```c
start:
if (![condition])
  goto end;
[statement]
goto start;
end:
```

Example:

```c
uint16 i = 0;
while (i < 10) {
  // do something
}
```

While loops are often used for more general terminating conditions that don't involve iteration.

### Do-While Loops

Syntax:

```c
do
  [statement]
while ([condition]);
```

Equivalent to:

```c
start:
[statement]
if ([condition])
  goto start;
```

Example:

```c
uint16 i = 10;
do {
  // something that should be done at least once
} while (i < 10);
```

Do while loops will execute the body of the loop at least once because they execute the loop body *before* evaluating the condition.

### Break Statement

If used in the body of a loop, the `break;` statement will exit the loop regardless of the exit condition.

### Continue Statement

If used in the body of a loop, the `continue;` statement will cause any subsequent statements in the body of the loop not to be executed. A for loop will jump immediately to the `[final-statement]`, and a while or do-while loop will immediately evaluate the `[condition]` again.

## Functions

Valid function names have the same rules as valid variable names. Functions must be defined at the time of declaration, and can only be declared in the global scope. Function calls may reference functions declared either before or after the function call is made.

Syntax:

```c
[return-type] [function-name] ([param1], [param2], ...) {
  [function-body]
}
```

The `[return-type]` can be either a primitive data type or `void`, which indicates no return value. Array return types are not allowed. If the return type of a function is not `void` and the function does not contain a `return` statement for every possible code path, the compiler will not catch this and the return value will be garbage. In a `void` function, using the `return;` statement will leave the function without executing any more statements.

Each parameter has a primitive data type followed by a variable name. Arrays can be passed as an address pointing to the start of the array, with another parameter indicating the array's size.

Example 1:

```c
int16 cmp(int16 a, int16 b) {
  if (a < b) {
    return -1;
  } else if (b < a) {
    return 1;
  }
  return 0;
}
```

Example 2:

```c
void paint_rectangle(uint16 color,
                     uint16 x,     uint16 y,
                     uint16 width, uint16 height) {
  int16 i, j;
  COLOR(color);
  for (i = x; i < x + width; i = i + 1) {
  	for (j = y; j < y + height; j = j + 1) {
      PIXEL(x, y);
    }
  }
}
```

### Entry Point

The entry point of a program in Consolite C is a `main()` function, which takes no arguments and has a return type of void. The entry point cannot be called recursively, and it cannot be called from any other point in the program. For example:

```c
void main() {
  // Program code here
}
```

### Builtin Functions

* `void COLOR(uint16 color)` Sets the drawing color to the lower 8 bits of the given value.
* `void PIXEL(uint16 x, uint16 y)`Draws a pixel to the screen at the given x and y coordinates, using only the lower 8 bits of the coordinates.
* `void TIMERST()` Resets the system time to zero.
* `uint16 TIME()` Returns the time in milliseconds since the last call to `TIMERST()`, modulo 2^16.
* `uint16 INPUT(uint16 input_id)` Returns the value of the input at the given `input_id`.
* `uint16 RND()` Returns a random 16-bit value.

## Operators

### Arithmetic

* Unary `-`, negates a number.
* Binary `+`, adds two numbers together.
* Binary `-`, subtracts two numbers.
* Binary `*`, multiplies two numbers.
* Binary `/`, divides two numbers.
* Binary `%`, gets the remainder of division between two numbers. `a = b % c;` is equivalent to `a = b - (c * (b / c));`.

### Bitwise

* Unary `~`, gives the bit complement of a number.
* Binary `&`, gives the bitwise and of two numbers.
* Binary `|`, gives the bitwise or of two numbers.
* Binary `^`, gives the bitwise xor of two numbers.
* Binary `>>`, gives the left value arithmetically shifted to the right by the right value.
* Binary `<<`, gives the left value shifted to the left by the right value.

### Boolean

* Unary `!`, yields 1 if the operand is 0, or 0 otherwise.
* Binary `&&`, yields 1 if the operands are both true, or 0 otherwise. Does not short-circuit; the expression `func1() && func2()` will call `func2()` even if `func1()` returns false.
* Binary `||`, yields 1 if at least one of the operands is true, or 0 otherwise. Does not short-circuit; the expression `func1() || func2()` will call `func2()` even if `func1()` returns true.
* Binary `<`, yields 1 if the first operand is less than the second. If one or both of the operands is a signed type, both of the operands will be treated as signed.
* Binary `<=`, yields 1 if the first operand is less than or equal to the second. If one or both of the operands is a signed type, both of the oeprands will be treated as signed.
* Binary `>`, yields 1 if the first operand is greater than the second. If one or both of the operands is a signed type, both of the oeprands will be treated as signed.
* Binary `>=`, yields 1 if the first operand is greater than or equal to the second. If one or both of the operands is a signed type, both of the oeprands will be treated as signed.
* Binary `==`, yields 1 if the bit patterns of the two operands are identical.
* Binary `!=`, yields 1 if the bit patterns of the two operands differ.

### Addresses

* Unary `&`, yields the address of the given variable.
* Unary `*`, yields the value at the address stored in the given variable. If used on the left hand side of an assignment statement such as `*x = 0;`, then the value is written to the location that x points to, rather than x itself.
