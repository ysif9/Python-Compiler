cmake_minimum_required(VERSION 3.30)
project(Python_Compiler)

set(CMAKE_CXX_STANDARD 20)

add_executable(Python_Compiler main.cpp
        Lexer.cpp)
