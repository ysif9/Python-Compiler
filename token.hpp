#pragma once
#include <iostream>


enum class TokenType {
    // Keywords
    TK_IF,
    TK_ELSE,
    TK_FOR,
    TK_WHILE,
    TK_DEF,
    TK_RETURN,
    TK_FALSE,
    TK_NONE,
    TK_TRUE,
    TK_AND,
    TK_AS,
    TK_ASSERT,
    TK_ASYNC,
    TK_AWAIT,
    TK_BREAK,
    TK_CLASS,
    TK_CONTINUE,
    TK_DEL,
    TK_ELIF,
    TK_EXCEPT,
    TK_FINALLY,
    TK_FROM,
    TK_GLOBAL,
    TK_IMPORT,
    TK_IN,
    TK_IS,
    TK_LAMBDA,
    TK_NONLOCAL,
    TK_NOT,
    TK_OR,
    TK_PASS,
    TK_RAISE,
    TK_TRY,
    TK_WITH,
    TK_YIELD,
    // Identifier
    TK_IDENTIFIER,
    // Constants
    TK_NUMBER,
    TK_STRING,
    // Operators
    TK_PLUS,
    TK_MINUS,
    TK_MULTIPLY,
    TK_DIVIDE,
    TK_FLOORDIV,
    TK_FLOORDIV_ASSIGN,
    TK_MOD,
    TK_MOD_ASSIGN,
    TK_POWER,
    TK_POWER_ASSIGN,
    TK_BIT_AND,
    TK_BIT_AND_ASSIGN,
    TK_BIT_OR,
    TK_BIT_OR_ASSIGN,
    TK_BIT_XOR,
    TK_BIT_XOR_ASSIGN,
    TK_BIT_NOT,
    TK_BIT_RIGHT_SHIFT,
    TK_BIT_RIGHT_SHIFT_ASSIGN,
    TK_BIT_LEFT_SHIFT,
    TK_BIT_LEFT_SHIFT_ASSIGN,
    TK_ASSIGN,
    TK_PLUS_ASSIGN,
    TK_MINUS_ASSIGN,
    TK_MULTIPLY_ASSIGN,
    TK_DIVIDE_ASSIGN,
    TK_EQUAL,
    TK_NOT_EQUAL,
    TK_GREATER,
    TK_LESS,
    TK_GREATER_EQUAL,
    TK_LESS_EQUAL,
    TK_MATMUL,
    TK_IMATMUL,
    TK_WALNUT,
    TK_FUNC_RETURN_TYPE,
    // Punctuation
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_LBRACE,
    TK_RBRACE,
    TK_COMMA,
    TK_SEMICOLON,
    TK_COLON,
    TK_PERIOD,
    // End-of-file
    TK_EOF,
    TK_UNKNOWN
};


struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

inline std::string tokenTypeToString(const TokenType type) {
    switch(type) {
        // Keywords
        case TokenType::TK_IF: return "if";
        case TokenType::TK_ELSE: return "else";
        case TokenType::TK_FOR: return "for";
        case TokenType::TK_WHILE: return "while";
        case TokenType::TK_DEF: return "def";
        case TokenType::TK_RETURN: return "return";
        case TokenType::TK_FALSE: return "false";
        case TokenType::TK_NONE: return "None";
        case TokenType::TK_TRUE: return "true";
        case TokenType::TK_AND: return "and";
        case TokenType::TK_AS: return "as";
        case TokenType::TK_ASSERT: return "assert";
        case TokenType::TK_ASYNC: return "async";
        case TokenType::TK_AWAIT: return "await";
        case TokenType::TK_BREAK: return "break";
        case TokenType::TK_CLASS: return "class";
        case TokenType::TK_CONTINUE: return "continue";
        case TokenType::TK_DEL: return "del";
        case TokenType::TK_ELIF: return "elif";
        case TokenType::TK_EXCEPT: return "except";
        case TokenType::TK_FINALLY: return "finally";
        case TokenType::TK_FROM: return "from";
        case TokenType::TK_GLOBAL: return "global";
        case TokenType::TK_IMPORT: return "import";
        case TokenType::TK_IN: return "in";
        case TokenType::TK_IS: return "is";
        case TokenType::TK_LAMBDA: return "lambda";
        case TokenType::TK_NONLOCAL: return "nonlocal";
        case TokenType::TK_NOT: return "not";
        case TokenType::TK_OR: return "or";
        case TokenType::TK_PASS: return "pass";
        case TokenType::TK_RAISE: return "raise";
        case TokenType::TK_TRY: return "try";
        case TokenType::TK_WITH: return "with";
        case TokenType::TK_YIELD: return "yield";

        // Identifier
        case TokenType::TK_IDENTIFIER: return "identifier";

        // Constants
        case TokenType::TK_NUMBER: return "number";
        case TokenType::TK_STRING: return "string";

        // Operators
        case TokenType::TK_PLUS: return "+";
        case TokenType::TK_MINUS: return "-";
        case TokenType::TK_MULTIPLY: return "*";
        case TokenType::TK_DIVIDE: return "/";
        case TokenType::TK_FLOORDIV: return "//";
        case TokenType::TK_FLOORDIV_ASSIGN: return "//=";
        case TokenType::TK_MOD: return "%";
        case TokenType::TK_MOD_ASSIGN: return "%=";
        case TokenType::TK_POWER: return "**";
        case TokenType::TK_POWER_ASSIGN: return "**=";
        case TokenType::TK_BIT_AND: return "&";
        case TokenType::TK_BIT_AND_ASSIGN: return "&=";
        case TokenType::TK_BIT_OR: return "|";
        case TokenType::TK_BIT_OR_ASSIGN: return "|=";
        case TokenType::TK_BIT_XOR: return "^";
        case TokenType::TK_BIT_XOR_ASSIGN: return "^=";
        case TokenType::TK_BIT_NOT: return "~";
        case TokenType::TK_BIT_RIGHT_SHIFT: return ">>";
        case TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN: return ">>=";
        case TokenType::TK_BIT_LEFT_SHIFT: return "<<";
        case TokenType::TK_BIT_LEFT_SHIFT_ASSIGN: return "<<=";
        case TokenType::TK_ASSIGN: return "=";
        case TokenType::TK_PLUS_ASSIGN: return "+=";
        case TokenType::TK_MINUS_ASSIGN: return "-=";
        case TokenType::TK_MULTIPLY_ASSIGN: return "*=";
        case TokenType::TK_DIVIDE_ASSIGN: return "/=";
        case TokenType::TK_EQUAL: return "==";
        case TokenType::TK_NOT_EQUAL: return "!=";
        case TokenType::TK_GREATER: return ">";
        case TokenType::TK_LESS: return "<";
        case TokenType::TK_GREATER_EQUAL: return ">=";
        case TokenType::TK_LESS_EQUAL: return "<=";
        case TokenType::TK_MATMUL: return "@";
        case TokenType::TK_IMATMUL: return "@=";
        case TokenType::TK_WALNUT: return ":=";
        case TokenType::TK_FUNC_RETURN_TYPE: return "->";

        // Punctuation
        case TokenType::TK_LPAREN: return "(";
        case TokenType::TK_RPAREN: return ")";
        case TokenType::TK_LBRACKET: return "[";
        case TokenType::TK_RBRACKET: return "]";
        case TokenType::TK_LBRACE: return "{";
        case TokenType::TK_RBRACE: return "}";
        case TokenType::TK_COMMA: return ",";
        case TokenType::TK_SEMICOLON: return ";";
        case TokenType::TK_COLON: return ":";
        case TokenType::TK_PERIOD: return ".";

        // End-of-file
        case TokenType::TK_EOF: return "EOF";
        case TokenType::TK_UNKNOWN: return "unknown";
    }
    return "invalid";


