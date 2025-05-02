#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

#include <string>
#include <utility>
#include <vector>
#include <memory>

class ExprAST {
public:
    virtual ~ExprAST() = default;
};

class NumberExprAST final : public ExprAST {
public:
    explicit NumberExprAST(const double value) : m_value(value) {}
private:
    double m_value;
};

class VariableExprAST final : public ExprAST {
public:
    explicit VariableExprAST(std::string  name) : m_name(std::move(name)) {}
private:
    std::string m_name;
};

class BinaryExprAST final : public ExprAST {
public:
    explicit BinaryExprAST(const char op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : m_op(op), m_LHS(std::move(LHS)), m_RHS(std::move(RHS)) {}
private:
    char m_op;
    std::unique_ptr<ExprAST> m_LHS, m_RHS;
};

class CallExprAST final : public ExprAST {
public:
    explicit CallExprAST(std::string  callee, std::vector<std::unique_ptr<ExprAST>> args)
        : m_callee(std::move(callee)), m_args(std::move(args)) {}
private:
    std::string m_callee;
    std::vector<std::unique_ptr<ExprAST>> m_args;
};

class PrototypeAST {
public:
    PrototypeAST(const std::string& name, std::vector<std::string> args)
        : m_name(name), m_args(std::move(args)) {}

    std::string getName() { return m_name; };
private:
    std::string m_name;
    std::vector<std::string> m_args;
};

class FunctionAST {
public:
    FunctionAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<ExprAST> body)
        : m_prototype(std::move(prototype)), m_body(std::move(body)) {}
private:
    std::unique_ptr<PrototypeAST> m_prototype;
    std::unique_ptr<ExprAST> m_body;
};

#endif //PARSER_UTILS_H
