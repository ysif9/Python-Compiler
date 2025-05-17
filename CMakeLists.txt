cmake_minimum_required(VERSION 3.26)

project(Python_Compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt setup

# Include directories
include_directories(
        include
)

# Source files
set(SOURCES
        main.cpp


        # Compiler backend
        Lexer/Lexer.cpp
        Parser/Parser.cpp
        Lexer/DOTGenerator.cpp

)

# Header files (for Qt's MOC)
set(HEADERS

        include/Lexer.hpp
        include/Token.hpp
        include/Parser.hpp
        include/DOTGenerator.hpp
        include/ASTNode.hpp
        include/Expressions.hpp
        include/Helpers.hpp
        include/Literals.hpp
        include/Statements.hpp
        include/tempAST.hpp
        include/UtilNodes.hpp
)

add_executable(Python_Compiler ${SOURCES} ${HEADERS})


# Optional macOS/iOS settings
set_target_properties(Python_Compiler PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)