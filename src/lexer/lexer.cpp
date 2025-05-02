#include "lexer.h"

int Lexer::getTok() {
    while (isspace(m_last_char))
        m_last_char = getchar();

    if (isalpha(m_last_char)) {
        m_ident = m_last_char;
        while (isalnum((m_last_char = getchar())))
            m_ident += m_last_char;

        if (m_ident == "def")
            return tok_def;
        if (m_ident == "extern")
            return tok_extern;
        return tok_identifier;
    }

    if (isdigit(m_last_char) || m_last_char == '.') {
        std::string num_str;
        do {
            num_str += m_last_char;
            m_last_char = getchar();
        } while (isdigit(m_last_char) || m_last_char == '.');

        m_number = strtod(num_str.c_str(), nullptr);
        return tok_number;
    }

    if (m_last_char == '#') {
        do
            m_last_char = getchar();
        while (m_last_char != EOF && m_last_char != '\n' && m_last_char != '\r');

        if (m_last_char != EOF)
            return getTok();
    }

    if (m_last_char == EOF)
        return tok_eof;

    int this_char = m_last_char;
    m_last_char = getchar();
    return this_char;
}

