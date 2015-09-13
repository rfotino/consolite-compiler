/**
 * Consolite Compiler
 * Copyright (c) 2015 Robert Fotino, All Rights Reserved
 */

#ifndef CONSOLITE_COMPILER_SYNTAX_H
#define CONSOLITE_COMPILER_SYNTAX_H

#include <string>
#include <vector>
#include <stack>
#include <memory>
#include "tokenizer.h"

class SyntaxToken {
 private:
  /**
   * The line number where the token starts in the input file, used
   * for helpful error reporting.
   */
  int _lineNum;
};

class GlobalVarToken : public SyntaxToken {
 public:
  GlobalVarToken(const std::string& type, const std::string& name)
    : _dataType(type), _varName(name) { }
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<GlobalVarToken>> &globals);
 private:
  std::string _dataType;
  std::string _varName;
  int _value;
};

class ParamToken : public SyntaxToken {
 private:
  std::string _type;
  std::string _name;
};

class StatementToken : public SyntaxToken {

};

class FunctionToken : public SyntaxToken {
 public:
  FunctionToken(const std::string& type, const std::string& name)
    : _returnType(type), _funcName(name) { }
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<GlobalVarToken>> &globals);
 private:
  std::string _returnType;
  std::string _funcName;
  std::vector<ParamToken> _parameters;
  std::vector<StatementToken> _statements;
};

class ExpressionToken : public SyntaxToken {
 public:
  bool isConst();
 private:
  std::stack<std::string> _symbols;
};

class ExpressionStatement : public StatementToken {
 private:
  ExpressionToken _expression;
};

class LocalVarStatement : public StatementToken {
 private:
  std::string _type;
  std::string _name;
  ExpressionToken _value;
  bool _hasValue;
};

class NullStatement : public StatementToken {

};

class CompoundStatement : public StatementToken {
 private:
  std::vector<StatementToken> _statements;
};

class IfStatement : public StatementToken {
 private:
  ExpressionToken _condExpression;
  StatementToken _trueStatement;
  StatementToken _falseStatement;
  bool _hasElse;
};

class LoopStatement : public StatementToken {
 private:
  ExpressionToken _condExpression;
  StatementToken _body;
};

class ForStatement : public LoopStatement {
 private:
  ExpressionToken _initExpression;
  ExpressionToken _loopExpression;
};

class WhileStatement : public LoopStatement {

};

class DoWhileStatement : public LoopStatement {

};

class BreakStatement : public StatementToken {

};

class ContinueStatement : public StatementToken {

};

class ReturnStatement : public StatementToken {
 private:
  ExpressionToken _returnValue;
  bool _hasValue;
};

class LabelStatement : public StatementToken {
 private:
  std::string _name;
};

class GotoStatement : public StatementToken {
 private:
  std::string _label;
};

#endif
