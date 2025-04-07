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

enum class TokenCategory {
    IDENTIFIER,
    KEYWORD,
    NUMBER,
    STRING,
    PUNCTUATION,
    OPERATOR,
    EOFILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    TokenCategory category;
};

inline std::string tokenTypeToString(const TokenType type) {
    switch (type) {
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

        // End-of-file
        case TokenType::TK_EOF: return "EOF";
        default: return "unknown";
    }
}

inline TokenCategory getTokenCategory(const TokenType type) {
    using TT = TokenType;
    switch (type) {
        // Keywords
        case TT::TK_IF:
        case TT::TK_ELSE:
        case TT::TK_FOR:
        case TT::TK_WHILE:
        case TT::TK_DEF:
        case TT::TK_RETURN:
        case TT::TK_FALSE:
        case TT::TK_NONE:
        case TT::TK_TRUE:
        case TT::TK_AND:
        case TT::TK_AS:
        case TT::TK_ASSERT:
        case TT::TK_ASYNC:
        case TT::TK_AWAIT:
        case TT::TK_BREAK:
        case TT::TK_CLASS:
        case TT::TK_CONTINUE:
        case TT::TK_DEL:
        case TT::TK_ELIF:
        case TT::TK_EXCEPT:
        case TT::TK_FINALLY:
        case TT::TK_FROM:
        case TT::TK_GLOBAL:
        case TT::TK_IMPORT:
        case TT::TK_IN:
        case TT::TK_IS:
        case TT::TK_LAMBDA:
        case TT::TK_NONLOCAL:
        case TT::TK_NOT:
        case TT::TK_OR:
        case TT::TK_PASS:
        case TT::TK_RAISE:
        case TT::TK_TRY:
        case TT::TK_WITH:
        case TT::TK_YIELD:
            return TokenCategory::KEYWORD;

        // Identifiers
        case TT::TK_IDENTIFIER:
            return TokenCategory::IDENTIFIER;

        // Constants
        case TT::TK_NUMBER:
            return TokenCategory::NUMBER;
        case TT::TK_STRING:
            return TokenCategory::STRING;

        // Punctuation
        case TT::TK_LPAREN:
        case TT::TK_RPAREN:
        case TT::TK_LBRACKET:
        case TT::TK_RBRACKET:
        case TT::TK_LBRACE:
        case TT::TK_RBRACE:
        case TT::TK_COMMA:
        case TT::TK_SEMICOLON:
        case TT::TK_COLON:
        case TT::TK_PERIOD:
            return TokenCategory::PUNCTUATION;

        // Operators
        case TT::TK_PLUS:
        case TT::TK_MINUS:
        case TT::TK_MULTIPLY:
        case TT::TK_DIVIDE:
        case TT::TK_FLOORDIV:
        case TT::TK_FLOORDIV_ASSIGN:
        case TT::TK_MOD:
        case TT::TK_MOD_ASSIGN:
        case TT::TK_POWER:
        case TT::TK_POWER_ASSIGN:
        case TT::TK_BIT_AND:
        case TT::TK_BIT_AND_ASSIGN:
        case TT::TK_BIT_OR:
        case TT::TK_BIT_OR_ASSIGN:
        case TT::TK_BIT_XOR:
        case TT::TK_BIT_XOR_ASSIGN:
        case TT::TK_BIT_NOT:
        case TT::TK_BIT_RIGHT_SHIFT:
        case TT::TK_BIT_RIGHT_SHIFT_ASSIGN:
        case TT::TK_BIT_LEFT_SHIFT:
        case TT::TK_BIT_LEFT_SHIFT_ASSIGN:
        case TT::TK_ASSIGN:
        case TT::TK_PLUS_ASSIGN:
        case TT::TK_MINUS_ASSIGN:
        case TT::TK_MULTIPLY_ASSIGN:
        case TT::TK_DIVIDE_ASSIGN:
        case TT::TK_EQUAL:
        case TT::TK_NOT_EQUAL:
        case TT::TK_GREATER:
        case TT::TK_LESS:
        case TT::TK_GREATER_EQUAL:
        case TT::TK_LESS_EQUAL:
        case TT::TK_MATMUL:
        case TT::TK_IMATMUL:
        case TT::TK_WALNUT:
        case TT::TK_FUNC_RETURN_TYPE:
            return TokenCategory::OPERATOR;

        case TT::TK_EOF:
            return TokenCategory::EOFILE;

        default:
            return TokenCategory::UNKNOWN; // fallback for unknowns
    }
}
