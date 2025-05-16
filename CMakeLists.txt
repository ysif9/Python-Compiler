cmake_minimum_required(VERSION 3.26)
project(Python_Compiler)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

add_executable(Python_Compiler main.cpp
        Lexer/Lexer.cpp
        include/Token.hpp
        include/Lexer.hpp
        include/UtilNodes.hpp
        include/ASTNode.hpp
        include/Expressions.hpp
        include/Literals.hpp
        include/Statements.hpp
        include/Helpers.hpp
        Parser/Parser.cpp
        include/Parser.hpp)
