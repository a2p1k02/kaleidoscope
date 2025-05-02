cmake_minimum_required(VERSION 3.28)

project(kaleidoscope)

add_executable(kaleidoscope
    src/lexer/lexer.h
    src/lexer/lexer.cpp
    src/parser/parser.h
    src/parser/parser.cpp
    src/main.cpp
        src/parser/parser_utils.hpp
)

