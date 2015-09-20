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

// Forward declaration, some tokens take a pointer to a tokenizer as an
// argument to parse().
class Tokenizer;

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

/**
 * A token representing a global variable. This includes the name, type,
 * and initial value of the global. The parse() function is for parsing
 * declarations like "uint16[3] glob = { 1, 2, 3 };" when we are in
 * the global scope.
 */
class GlobalVarToken : public Token {
 public:
  GlobalVarToken(const TypeToken& type, const std::string& name)
    : _type(type), _name(name) { }
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals);
  TypeToken type() const { return _type; }
  std::string name() const { return _name; }
  uint16_t val() const { return _value; }
  bool isArray() const { return _type.isArray(); }
  uint16_t arraySize() const { return _arrayValues.size(); }
  uint16_t arrayVal(int i) const { return _arrayValues.at(i); }
 private:
  TypeToken _type;
  std::string _name;
  uint16_t _value;
  std::vector<uint16_t> _arrayValues;
};

/**
 * A token representing a type and a name of a parameter for a function.
 */
class ParamToken : public Token {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals);
  TypeToken type() const { return _type; }
  std::string name() const { return _name; }
 private:
  TypeToken _type;
  std::string _name;
};

/**
 * A token representing a function definition. Has a return type,
 * a list of parameters, and a list of top-level statements. The parse()
 * function separates the parameters and statements into their own tokens.
 */
class FunctionToken : public Token,
                      public std::enable_shared_from_this<FunctionToken> {
 public:
  FunctionToken(const TypeToken& type, const std::string& name)
    : _type(type), _name(name) { }
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<FunctionToken>>& functions,
             std::vector<std::shared_ptr<GlobalVarToken>>& globals);
  TypeToken type() const { return _type; }
  std::string name() const { return _name; }
 private:
  TypeToken _type;
  std::string _name;
  std::vector<std::shared_ptr<ParamToken>> _parameters;
  std::vector<std::shared_ptr<LocalVarToken>> _localVars;
  std::vector<std::shared_ptr<LabelStatement>> _labels;
  std::vector<std::shared_ptr<StatementToken>> _statements;
};

/**
 * A token representing an expression like "(a+b)*(c-d)". It parses out an
 * expression tree of operators, literals, variables, and function calls
 * that can be checked for const-ness and evaluated.
 */
class ExprToken : public Token {
 public:
  ExprToken() : _const(true), _value(0) { }
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters = {},
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars = {});
  bool isConst() const { return _const; }
  uint16_t val() const { return _value; }
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
        const std::shared_ptr<FunctionToken>& currentFunc,
        bool inLoop = false);
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
             const std::shared_ptr<FunctionToken>& currentFunc,
             bool inLoop);
 private:
  std::vector<std::shared_ptr<StatementToken>> _statements;
};

/**
 * A token that represents a local variable declaration.
 */
class LocalVarToken : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<FunctionToken>>& functions,
             const std::vector<std::shared_ptr<GlobalVarToken>>& globals,
             const std::vector<std::shared_ptr<ParamToken>>& parameters,
             const std::vector<std::shared_ptr<LocalVarToken>>& localVars);
  std::string name() const { return _name; }
 private:
  TypeToken _type;
  std::string _name;
  /**
   * The initialization expressions. This will be an empty vector
   * if there was no initialization, or a single value if _type is
   * not an array, or one or more values if _type is an array.
   */
  std::vector<std::shared_ptr<ExprToken>> _initExprs;
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
 private:
  ExprToken _expr;
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
             const std::shared_ptr<FunctionToken>& currentFunc,
             bool inLoop);
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
 private:
  ExprToken _condExpr;
  StatementToken _body;
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
             const std::shared_ptr<FunctionToken>& currentFunc);
 private:
  ExprToken _initExpr;
  ExprToken _loopExpr;
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
             const std::shared_ptr<FunctionToken>& currentFunc);
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
             const std::shared_ptr<FunctionToken>& currentFunc);
};

/**
 * A token representing a break statement for breaking out
 * of a loop.
 */
class BreakStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer, bool inLoop);
};

/**
 * A token representing a continue statement for skipping the
 * rest of the current loop iteration.
 */
class ContinueStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer, bool inLoop);
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
 private:
  ExprToken _returnValue;
  bool _hasValue;
};

/**
 * A token representing a label declaration that can be jumped to
 * with a goto statement. Looks like a name followed by a colon.
 */
class LabelStatement : public StatementToken,
                       public std::enable_shared_from_this<LabelStatement> {
 public:
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<LabelStatement>>& labels);
  std::string name() const { return _name; }
 private:
  std::string _name;
};

/**
 * A token representing a goto statement, which jumps to a label
 * declaration.
 */
class GotoStatement : public StatementToken {
 public:
  bool parse(Tokenizer *tokenizer,
             const std::vector<std::shared_ptr<LabelStatement>>& labels);
 private:
  std::shared_ptr<LabelStatement> _label;
};

#endif
