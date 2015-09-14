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

class GlobalVarToken;
class FunctionToken;

class TypeToken {
 public:
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<FunctionToken>> &functions,
             std::vector<std::shared_ptr<GlobalVarToken>> &globals);
  std::string name() const { return _name; }
  bool isArray() const { return _isArray; }
  int arraySize() const { return _arraySize; }
  int line() const { return _lineNum; }
 private:
  std::string _name;
  bool _isArray;
  int _arraySize;
  int _lineNum;
};

class GlobalVarToken {
 public:
  GlobalVarToken(const TypeToken& type, const std::string& name)
    : _type(type), _name(name) { }
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<FunctionToken>> &functions,
             std::vector<std::shared_ptr<GlobalVarToken>> &globals);
  TypeToken type() const { return _type; }
  std::string name() const { return _name; }
 private:
  TypeToken _type;
  std::string _name;
  int _value;
  std::vector<int> _arrayValues;
};

class ParamToken {
 private:
  std::string _type;
  std::string _name;
};

class StatementToken {

};

class FunctionToken {
 public:
  FunctionToken(const TypeToken& type, const std::string& name)
    : _type(type), _name(name) { }
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<FunctionToken>> &functions,
             std::vector<std::shared_ptr<GlobalVarToken>> &globals);
  TypeToken type() const { return _type; }
  std::string name() const { return _name; }
 private:
  TypeToken _type;
  std::string _name;
  std::vector<ParamToken> _parameters;
  std::vector<StatementToken> _statements;
};

class ExprToken {
 public:
  ExprToken() : _const(true), _lineNum(-1) { }
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<FunctionToken>> &functions,
             std::vector<std::shared_ptr<GlobalVarToken>> &globals);
  bool isConst() const { return _const; }
  int constVal(std::vector<std::shared_ptr<GlobalVarToken>> &globals) const;
  int line() const { return _lineNum; }
 private:
  bool _const;
  int _lineNum;
  std::stack<std::string> _symbols;
};

class ArrayExprToken {
 public:
  ArrayExprToken() : _lineNum(-1) { }
  bool parse(Tokenizer *tokenizer,
             std::vector<std::shared_ptr<FunctionToken>> &functions,
             std::vector<std::shared_ptr<GlobalVarToken>> &globals);
  int size() const { return _exprs.size(); }
  ExprToken get(int i) const { return _exprs[i]; }
  int line() const { return _lineNum; }
 private:
  std::vector<ExprToken> _exprs;
  int _lineNum;
};

class LocalVarToken {
 private:
  std::string _type;
  std::string _name;
  ExprToken _value;
  bool _hasValue;
};

class ExprStatement : public StatementToken {
 private:
  ExprToken _expr;
};

class NullStatement : public StatementToken {

};

class CompoundStatement : public StatementToken {
 private:
  std::vector<StatementToken> _statements;
};

class IfStatement : public StatementToken {
 private:
  ExprToken _condExpr;
  StatementToken _trueStatement;
  StatementToken _falseStatement;
  bool _hasElse;
};

class LoopStatement : public StatementToken {
 private:
  ExprToken _condExpr;
  StatementToken _body;
};

class ForStatement : public LoopStatement {
 private:
  ExprToken _initExpr;
  ExprToken _loopExpr;
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
  ExprToken _returnValue;
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
