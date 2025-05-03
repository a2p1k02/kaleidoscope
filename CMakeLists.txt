cmake_minimum_required(VERSION 3.28)
project(kaleidoscope LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

llvm_map_components_to_libnames(llvm_libs
        core
        support
        irreader
        executionengine
        orcjit
        native
        passes
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(kaleidoscope
        src/lexer/lexer.h
        src/lexer/lexer.cpp
        src/parser/parser.h
        src/parser/parser.cpp
        src/main.cpp
        src/parser/parser_utils.hpp
)

target_include_directories(kaleidoscope PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(kaleidoscope PRIVATE
        ${llvm_libs}
)
