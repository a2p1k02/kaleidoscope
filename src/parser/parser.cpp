#include "parser.h"

#include "../lexer/lexer.h"

Parser::Parser() : m_current_tok(0) {
    binops['<'] = 10;
    binops['+'] = 20;
    binops['-'] = 20;
    binops['*'] = 40;

    fprintf(stderr, "ready> ");
    getNextToken();

    m_context = std::make_unique<llvm::LLVMContext>();
    m_module = std::make_unique<llvm::Module>("jit", *m_context);

    m_builder = std::make_unique<llvm::IRBuilder<>>(*m_context);
}

int Parser::getNextToken() {
    return m_current_tok = m_lexer.getTok();
}

int Parser::getTokPrecedence() {
    if (!isascii(m_current_tok))
        return -1;

    int tok = binops[m_current_tok];
    if (tok <= 0) return -1;
    return tok;
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
    auto LHS = parsePrimary();
    if (!LHS) {
        return nullptr;
    }
    return parseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ExprAST> Parser::parseNumberExpr() {
    auto result = std::make_unique<NumberExprAST>(m_lexer.getNumber());
    getNextToken();
    return std::move(result);
}

std::unique_ptr<ExprAST> Parser::parseParenExpr() {
    getNextToken();
    auto v = parseExpression();
    if (!v) return nullptr;

    if (m_current_tok != ')') return logError("expected ')'");
    getNextToken();
    return  v;
}

std::unique_ptr<ExprAST> Parser::parseIdentifierExpr() {
    std::string id_name = m_lexer.getIdent();

    getNextToken();

    if (m_current_tok != '(')
        return std::make_unique<VariableExprAST>(id_name);

    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> args;
    if (m_current_tok != ')') {
        while (true) {
            if (auto arg = parseExpression())
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (m_current_tok == ')')
                break;

            if (m_current_tok != ',')
                return logError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    getNextToken();

    return std::make_unique<CallExprAST>(id_name, std::move(args));
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    switch (m_current_tok) {
        default:
            return logError("unknown token when expecting an expression");
        case tok_identifier:
            return parseIdentifierExpr();
        case tok_number:
            return parseNumberExpr();
        case '(':
            return parseParenExpr();
    }
}

std::unique_ptr<ExprAST> Parser::parseBinOpRHS(int expr_prec, std::unique_ptr<ExprAST> lhs) {
    while (true) {
        int tok_prec = getTokPrecedence();

        if (tok_prec < expr_prec)
            return lhs;

        int bin_op = m_current_tok;
        getNextToken(); // eat binop

        auto rhs = parsePrimary();
        if (!rhs)
            return nullptr;

        int next_prec = getTokPrecedence();
        if (tok_prec < next_prec) {
            rhs = parseBinOpRHS(tok_prec + 1, std::move(rhs));
            if (!rhs)
                return nullptr;
        }

        lhs = std::make_unique<BinaryExprAST>(bin_op, std::move(lhs), std::move(rhs));
    }
}

std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
    if (m_current_tok != tok_identifier)
        return logErrorP("Expected function name in prototype");

    std::string fn_name = m_lexer.getIdent();
    getNextToken();

    if (m_current_tok != '(')
        return logErrorP("Expected '(' in prototype");

    std::vector<std::string> arg_names;
    while (getNextToken() == tok_identifier)
        arg_names.push_back(m_lexer.getIdent());
    if (m_current_tok != ')')
        return logErrorP("Expected ')' in prototype");

    getNextToken();

    return std::make_unique<PrototypeAST>(fn_name, std::move(arg_names));
}

std::unique_ptr<FunctionAST> Parser::parseDefinition() {
    getNextToken(); // eat def.
    auto proto = parsePrototype();
    if (!proto)
        return nullptr;

    if (auto e = parseExpression())
        return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
    return nullptr;
}

std::unique_ptr<FunctionAST> Parser::parseTopLevelExpr() {
    if (auto e = parseExpression()) {
        auto proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
    }
    return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::parseExtern() {
    getNextToken();
    return parsePrototype();
}

void Parser::handleDefinition() {
    if (auto fn_ast = parseDefinition()) {
        if (auto* fn_ir = fn_ast->codegen()) {
            fprintf(stderr, "Parsed a function definition:");
            fn_ir->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

void Parser::handleExtern() {
    if (auto proto_ast = parseExtern()) {
        if (auto* fn_ir = proto_ast->codegen()) {
            fprintf(stderr, "Parsed an extern:");
            fn_ir->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

void Parser::handleTopLevelExpression() {
    if (auto fn_ast = parseTopLevelExpr()) {
        if (auto* fn_ir = fn_ast->codegen()) {
            fprintf(stderr, "Parsed a top-level expr:");
            fn_ir->print(llvm::errs());
            fprintf(stderr, "\n");

            fn_ir->eraseFromParent();
        }
    } else {
        getNextToken();
    }
}

void Parser::run() {
    while (m_current_tok != tok_eof) {
        fprintf(stderr, "ready> ");
        switch (m_current_tok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                handleDefinition();
                break;
            case tok_extern:
                handleExtern();
                break;
            default:
                handleTopLevelExpression();
                break;
        }
    }
    m_module->print(llvm::errs(), nullptr);
}

