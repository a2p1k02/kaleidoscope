#ifndef LEXER_H
#define LEXER_H

#include <iostream>

enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5,
};

class Lexer {
public:
    Lexer() : m_last_char(' '), m_number(0) {}
    int getTok();

    double getNumber() const { return m_number; }
    const std::string& getIdent() const { return m_ident; }
private:
    std::string m_ident;
    int m_last_char;
    double m_number;
};

#endif //LEXER_H
