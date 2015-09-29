/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_SYNTAX_H
#define CONSOLITE_COMPILER_SYNTAX_H

#include <string>
#include <vector>
#include <memory>
#include <stack>

// Forward declaration, some tokens take a pointer to a tokenizer or
// parser as an argument.
class Tokenizer;
class Parser;

/**
 * A class for operands, used for expression evaluation.
 */
enum OperandType { ADDRESS, REGISTER, VALUE, LITERAL };
class Operand {
 public:
  Operand() { }
  Operand(OperandType type, const std::string& reg = "")
    : _type(type), _reg(reg) { }
  Operand(OperandType type, uint16_t literal)
    : _type(type), _literal(literal) { }
  OperandType type() const { return _type; }
  std::string reg() const { return _reg; }
  uint16_t literal() const { return _literal; }
 private:
  OperandType _type;
  std::string _reg;
  uint16_t _literal;
};

/**
 * The base class for all syntax tokens, has a line number and an
 * overridable function for getting the "value" of this token.
 */
class Token {
 public:
  Token() : _lineNum(-1) { }
  int line() const { return _lineNum; }
  virtual uint16_t val() const { return 0; }
 protected:
  int _lineNum;
};

/**
 * The type of token returned by the tokenizer, could be a symbol,
 * a name, an operator, etc, represented by an undifferentiated string.
 */
class AtomToken : public Token {
 public:
  AtomToken() { }
  AtomToken(const std::string& strVal, int lineNum)
    : _str(strVal) { _lineNum = lineNum; }
  std::string str() const { return _str; }
  bool empty() const { return _str.empty(); }
 private:
  std::string _str;
};

class GlobalVarToken;
class FunctionToken;
class ParamToken;
class LocalVarToken;
class StatementToken;
class LabelStatement;
class GotoStatement;

/**
 * A token representing a literal value from the code like "0x1234" or "4321".
 */
class LiteralToken : public Token {
 public:
  LiteralToken(uint16_t value = 0) : _value(value) { }
  bool parse(const AtomToken& token);
  uint16_t val() const { return _value; }
 private:
  uint16_t _value;
};

/**
 * A token representing a binary or unary operator in an expression, like
 * "+", "==", or "!".
 */
class OperatorToken : public Token {
 public:
  /**
   * Returns true if the token is a valid operator.
   */
  bool parse(const AtomToken& token);
  /**
   * Returns true if the token could be a binary operator. Some tokens like
   * "-", "&", and "*" can represent either unary or binary operations.
   */
  bool maybeBinary();
  /**
   * Returns true if the token could be a unary operator.
   */
  bool maybeUnary();
  /**
   * Sets the token to be a binary operator. This can be used after context
   * has been established so we know whether ambiguous operators are binary
   * or unary.
   */
  void setBinary() { _binary = true; }
  /**
   * Sets the token to be a unary operator.
   */
  void setUnary() { _binary = false; }
  /**
   * Returns true if the operator has been set to binary with setBinary().
   */
  bool isBinary() const { return _binary; }
  /**
   * Returns true if the operator has been set to unary with setUnary().
   */
  bool isUnary() const { return !_binary; }
  /**
   * Returns the precedence of this operator as an int.
   */
  int precedence() const;
  /**
   * Returns the associativity of this operator, either left-to-right (true)
   * or right-to-left (false).
   */
  bool leftToRight() const;
  /**
   * Returns the result of the operation with the given left hand and right
   * hand sides.
   */
  uint16_t operate(uint16_t lhs, uint16_t rhs) const;
  /**
   * Outputs assembly code for this operation on the given left hand and
   * right hand sides. Returns an operand representing the result, which
   * will be an address or a value depending on the operation.
   */
  Operand output(Parser *parser, const Operand& lhs, const Operand& rhs);
  /**
   * Returns a string representation of the operator.
   */
  std::string str() const { return _op; }
 private:
  std::string _op;
  bool _binary;
};

/**
 * A token representing a type. This could have been constructed from
 * a single token like "uint16" or from several like "uint16", "[", "3", "]"
 * if it is an array type,
 */
class TypeToken : public Token {
 public:
  TypeToken() : _isArray(false), _arraySize(-1) { }
  TypeToken(const std::string& name, bool isArray = false, size_t arraySize = 0)
    : _name(name), _isArray(isArray), _arraySize(arraySize) { }
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters = {},
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars = {});
  std::string name() const { return _name; }
  bool isArray() const { return _isArray; }
  size_t arraySize() const { return _arraySize; }
 private:
  std::string _name;
  bool _isArray;
  uint16_t _arraySize;
};

class VarLocation {
 public:
  VarLocation() { }
  VarLocation(const std::string& reg) : _reg(reg) { }
  VarLocation(int offset) : _offset(offset) { }
  VarLocation(const std::string& reg, int offset)
    : _reg(reg), _offset(offset) { }
  /**
   * Returns true if the variable's location is stored in a register.
   */
  bool isReg() const { return !_reg.empty(); }
  /**
   * Returns the assembly code representation of this register, like "A".
   */
  std::string getReg() const { return _reg; }
  /**
   * Sets the assembly code representation of this register, like "A".
   */
  void setReg(const std::string& reg) { _reg = reg; }
  /**
   * Returns the offset from the frame pointer in bytes at which this
   * variable is stored.
   */
  int getOffset() const { return _offset; }
  /**
   * Sets the offset from the frame pointer in bytes at which this
   * variable is stored.
   */
  void setOffset(int offset) { _offset = offset; }
 protected:
  /**
   * The assembly code representation of this register, or the empty
   * string if it is not stored in a register.
   */
  std::string _reg;
  /**
   * The offset from the frame pointer in bytes at which this variable
   * is stored, if it is not in a register.
   */
  int _offset;
};

/**
 * A class for storing information about a variable, such as the type,
 * name, and location as a register or frame offset.
 */
class Variable : public VarLocation {
 public:
  Variable() : _canBeReg(true) { }
  Variable(const TypeToken& type, const std::string& name)
    : _type(type), _name(name), _canBeReg(true) { }
  TypeToken type() const { return _type; }
  std::string name() const { return _name; }
  /**
   * Returns true if this variable can be stored in a register.
   */
  bool canBeReg() const { return _canBeReg; }
  /**
   * Flags this variable as not able to be stored in a register. This
   * could be because it was used with an address operator.
   */
  void flagNonReg() { _canBeReg = false; }
 protected:
  TypeToken _type;
  std::string _name;
  bool _canBeReg;
};

/**
 * A token representing a global variable. This includes the name, type,
 * and initial value of the global. The parse() function is for parsing
 * declarations like "uint16[3] glob = { 1, 2, 3 };" when we are in
 * the global scope.
 */
class GlobalVarToken : public Token, public Variable {
 public:
  GlobalVarToken(const TypeToken& type, const std::string& name)
    : Variable(type, name) { }
  /**
   * Parses out a global variable declaration from source code
   * and validates it.
   */
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals);
  /**
   * Outputs assembly code for this global variable declaration.
   */
  void output(Parser *parser);
  uint16_t val() const { return _value; }
  bool isArray() const { return _type.isArray(); }
  uint16_t arraySize() const { return _arrayValues.size(); }
  uint16_t arrayVal(int i) const { return _arrayValues.at(i); }
 private:
  uint16_t _value;
  std::vector<uint16_t> _arrayValues;
};

/**
 * A token representing a type and a name of a parameter for a function.
 */
class ParamToken : public Token, public Variable {
 public:
  ParamToken() { }
  ParamToken(const TypeToken& type, const std::string& name)
    : Variable(type, name) { }
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals);
};

/**
 * A token representing a function definition. Has a return type,
 * a list of parameters, and a list of top-level statements. The parse()
 * function separates the parameters and statements into their own tokens.
 */
class FunctionToken : public Token,
                      public std::enable_shared_from_this<FunctionToken> {
 public:
  FunctionToken(const TypeToken& type, const std::string& name,
                const std::vector<std::shared_ptr<ParamToken>>& params)
    : _type(type), _name(name), _parameters(params) { }
  FunctionToken(const TypeToken& type, const std::string& name)
    : _type(type), _name(name) { }
  /**
   * Parses source code for a function and validates it. Returns false
   * if there are errors in parsing the function.
   */
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<FunctionToken>>& functions,
             std::vector<std::shared_ptr<GlobalVarToken>>& globals);
  /**
   * Outputs assembly code for this function.
   */
  void output(Parser *parser);
  /**
   * Translates the given source-level label within this function into the
   * assembly-level label that has been assigned to it. Returns the empty
   * string if the source-level label does not exist.
   */
  std::string toAsmLabel(const std::string& srcLabel);
  TypeToken type() const { return _type; }
  std::string name() const { return _name; }
  size_t numParams() const { return _parameters.size(); }
  std::shared_ptr<ParamToken> getParam(int i) const { return _parameters.at(i); }
 private:
  TypeToken _type;
  std::string _name;
  std::vector<std::shared_ptr<ParamToken>> _parameters;
  std::vector<std::shared_ptr<LocalVarToken>> _localVars;
  std::vector<std::shared_ptr<LabelStatement>> _labels;
  std::vector<std::shared_ptr<GotoStatement>> _gotos;
  std::vector<std::shared_ptr<StatementToken>> _statements;
  std::stack<std::string> _savedRegisters;
};

/**
 * A token representing an expression like "(a+b)*(c-d)". It parses out an
 * expression tree of operators, literals, variables, and function calls
 * that can be checked for const-ness and evaluated.
 */
class ExprToken : public Token {
 public:
  ExprToken() : _const(true), _value(0) { }
  ExprToken(uint16_t value);
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters = {},
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars = {});
  bool isConst() const { return _const; }
  uint16_t val() const { return _value; }
  /**
   * Outputs assembly code to evaluate the given expression and store the
   * result in the given register, reg.
   */
  void output(Parser *parser, const VarLocation& varLoc);
 private:
  /**
   * Validates the expression to see if it will compile. Prints errors if
   * using an rvalue on the left side of an assignment, etc. Returns true
   * if the expression is valid, or false otherwise.
   */
  bool _validate();
  /**
   * Tries to evaluate the expression as if it were constant, setting _const
   * appropriately and _value if successful. Warns of divide by zero errors
   * etc.
   */
  void _evaluate();
  /**
   * Flags variables used in this expression that are not able to be stored
   * in registers due to address-of operations.
   */
  void _flagNonRegs();
  /**
   * Whether this expression is constant (meaning, in this context, known
   * at compile-time).
   */
  bool _const;
  /**
   * The constant value of this expression, as known at compile-time.
   */
  uint16_t _value;
  /**
   * A list of tokens in this expression, in postfix notation.
   */
  std::vector<std::shared_ptr<Token>> _postfix;
};

/**
 * A token representing an array of expressions separated by commas within
 * braces. Looks like "{1, 2, 3}", where 1, 2, and 3 are expression tokens.
 */
class ArrayExprToken : public Token {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters = {},
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars = {});
  size_t size() const { return _exprs.size(); }
  std::shared_ptr<ExprToken> get(int i) const { return _exprs[i]; }
 private:
  std::vector<std::shared_ptr<ExprToken>> _exprs;
};

/**
 * A token that represents a function call used in an expression.
 * it stores the function name and the parameter expressions.
 */
class FunctionCallToken : public Token {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars);
  /**
   * Outputs the assembly code for this function call.
   */
  void output(Parser *parser);
  std::string funcName() const { return _funcName; }
  size_t numArgs() const { return _arguments.size(); }
 private:
  std::string _funcName;
  std::vector<std::shared_ptr<ExprToken>> _arguments;
};

/**
 * A token that represents a statement inside of a function, such as
 * a local variable declaration, a for loop, an if statement, etc.
 */
class StatementToken : public Token {
 public:
  /**
   * Parses the next statement from the tokenizer and returns a pointer
   * to it. Returns a null pointer if the next statement isn't valid.
   */
  static std::shared_ptr<StatementToken> parse(
        Tokenizer *tokenizer,
        const std::vector<std::shared_ptr<FunctionToken>>& functions,
        const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
        const std::vector<std::shared_ptr<ParamToken>>& parameters,
        const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
        std::vector<std::shared_ptr<LabelStatement>>& labels,
        std::vector<std::shared_ptr<GotoStatement>>& gotos,
        const std::shared_ptr<FunctionToken>& currentFunc,
        bool inLoop = false);
  /**
   * Outputs the assembly code for this statement. Does nothing by
   * default, should be implemented by subclasses.
   */
  virtual void output(Parser *parser,
                      const std::shared_ptr<FunctionToken>& function,
                      const std::string& returnLabel,
                      const std::string& breakLabel = "",
                      const std::string& continueLabel = "");
};

/**
 * A token representing a list of statement tokens within curly braces.
 */
class CompoundStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
             std::vector<std::shared_ptr<LabelStatement>>& labels,
             std::vector<std::shared_ptr<GotoStatement>>& gotos,
             const std::shared_ptr<FunctionToken>& currentFunc,
             bool inLoop);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
 private:
  std::vector<std::shared_ptr<StatementToken>> _statements;
};

/**
 * A token that represents a local variable declaration.
 */
class LocalVarToken : public StatementToken, public Variable {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars);
  /**
   * If an initial value is set for this local variable, this outputs the
   * assembly code to initialize the variable.
   */
  void output(Parser *parser,
	      const std::shared_ptr<FunctionToken>& = nullptr,
	      const std::string& = "",
	      const std::string& = "",
	      const std::string& = "");
  /**
   * Sets the location of the start of data as an offset from the frame pointer,
   * used for array variables.
   */
  void setDataOffset(uint16_t dataOffset) { _dataOffset = dataOffset; }
 private:
  /**
   * The initialization expressions. This will be an empty vector
   * if there was no initialization, or a single value if _type is
   * not an array, or one or more values if _type is an array.
   */
  std::vector<std::shared_ptr<ExprToken>> _initExprs;
  /**
   * The location of the start of data as an offset from the frame pointer,
   * used for array variablefs.
   */
  uint16_t _dataOffset;
};

/**
 * A token representing an expression statement, which is an
 * expression followed by a semicolon. The expression can have side
 * effects if it includes assignment or function calls.
 */
class ExprStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
 private:
  ExprToken _expr;
};

/**
 * A token representing a void function call followed by a semicolon.
 */
class VoidStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
 private:
  FunctionCallToken _fnCall;
};

/**
 * A token representing a null statement, which is simply a ';' token.
 */
class NullStatement : public StatementToken {

};

/**
 * A token representing an if statement, with optional else statement.
 */
class IfStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
             std::vector<std::shared_ptr<LabelStatement>>& labels,
             std::vector<std::shared_ptr<GotoStatement>>& gotos,
             const std::shared_ptr<FunctionToken>& currentFunc,
             bool inLoop);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
 private:
  ExprToken _condExpr;
  std::shared_ptr<StatementToken> _trueStatement;
  std::shared_ptr<StatementToken> _falseStatement;
  bool _hasElse;
};

/**
 * A token representing a generic loop.
 */
class LoopStatement : public StatementToken {
 protected:
  std::shared_ptr<ExprToken> _condExpr;
  std::shared_ptr<StatementToken> _body;
};

/**
 * A token representing a for loop.
 */
class ForStatement : public LoopStatement {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
             std::vector<std::shared_ptr<LabelStatement>>& labels,
             std::vector<std::shared_ptr<GotoStatement>>& gotos,
             const std::shared_ptr<FunctionToken>& currentFunc);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
 private:
  std::vector<std::shared_ptr<ExprToken>> _initExprs;
  std::vector<std::shared_ptr<ExprToken>> _loopExprs;
};

/**
 * A token representing a while loop.
 */
class WhileStatement : public LoopStatement {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
             std::vector<std::shared_ptr<LabelStatement>>& labels,
             std::vector<std::shared_ptr<GotoStatement>>& gotos,
             const std::shared_ptr<FunctionToken>& currentFunc);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
};

/**
 * A token representing a do-while loop.
 */
class DoWhileStatement : public LoopStatement {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
             std::vector<std::shared_ptr<LabelStatement>>& labels,
             std::vector<std::shared_ptr<GotoStatement>>& gotos,
             const std::shared_ptr<FunctionToken>& currentFunc);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
};

/**
 * A token representing a break statement for breaking out
 * of a loop.
 */
class BreakStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer, bool inLoop);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
};

/**
 * A token representing a continue statement for skipping the
 * rest of the current loop iteration.
 */
class ContinueStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer, bool inLoop);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
};

/**
 * A token representing a return statement, possibly with an
 * expression representing a return value.
 */
class ReturnStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars,
             const std::shared_ptr<FunctionToken>& currentFunc);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
 private:
  ExprToken _returnExpr;
  bool _hasExpr;
};

/**
 * A token representing a label declaration that can be jumped to
 * with a goto statement. Looks like a name followed by a colon.
 */
class LabelStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
  /**
   * Returns the source-level label.
   */
  std::string name() const { return _name; }
  /**
   * Sets the assembly-level label used for jumping to this
   * label statement.
   */
  void setAsmLabel(const std::string& label) { _asmLabel = label; }
  /**
   * Returns the assembly-level label.
   */
  std::string getAsmLabel() const { return _asmLabel; }
 private:
  std::string _name;
  std::string _asmLabel;
};

/**
 * A token representing a goto statement, which jumps to a label
 * declaration.
 */
class GotoStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer);
  /**
   * Outputs the assembly code for this statement.
   */
  void output(Parser *parser,
              const std::shared_ptr<FunctionToken>& function,
              const std::string& returnLabel,
              const std::string& breakLabel,
              const std::string& continueLabel);
  /**
   * Returns the source-level label.
   */
  std::string label() const { return _label; }
 private:
  std::string _label;
};

#endif
