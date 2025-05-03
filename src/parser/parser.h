#ifndef PARSER_H
#define PARSER_H

#include <map>

#include "parser_utils.hpp"
#include "../lexer/lexer.h"

class Parser {
public:
    explicit Parser();

    void handleDefinition();
    void handleExtern();
    void handleTopLevelExpression();
    void run();
private:
    //Fields
    int m_current_tok;
    Lexer m_lexer;
    std::map<char, int> binops;

    //Methods
    int getNextToken();
    int getTokPrecedence();

    std::unique_ptr<ExprAST> parseExpression();
    std::unique_ptr<ExprAST> parseNumberExpr();
    std::unique_ptr<ExprAST> parseParenExpr();
    std::unique_ptr<ExprAST> parseIdentifierExpr();
    std::unique_ptr<ExprAST> parsePrimary();
    std::unique_ptr<ExprAST> parseBinOpRHS(int expr_prec, std::unique_ptr<ExprAST> lhs);
    std::unique_ptr<PrototypeAST> parsePrototype();
    std::unique_ptr<FunctionAST> parseDefinition();
    std::unique_ptr<FunctionAST> parseTopLevelExpr();
    std::unique_ptr<PrototypeAST> parseExtern();
};

#endif //PARSER_H
