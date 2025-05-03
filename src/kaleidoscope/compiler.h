#ifndef COMPILER_H
#define COMPILER_H

#include "../parser/parser.h"

/*
 * Class for next updates
 * because it's not okay to contain all things in parser.
 */

class Compiler {
public:
    explicit Compiler();
private:
    Parser m_parser;
};

#endif //COMPILER_H
