#pragma once
#include <iostream>
#include <string> // Added for string usage in SymbolInfo

using namespace std;

// --- Token Types ---
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
    TK_STR,         // Type keyword
    TK_INT,         // Type keyword
    TK_FLOAT,       // Type keyword
    TK_COMPLEX,     // Type keyword AND Literal type
    TK_LIST,        // Type keyword
    TK_TUPLE,       // Type keyword
    TK_RANGE,       // Type keyword
    TK_DICT,        // Type keyword
    TK_SET,         // Type keyword
    TK_FROZENSET,   // Type keyword
    TK_BOOL,        // Type keyword
    TK_BYTES,       // Type keyword AND Literal type
    TK_BYTEARRAY,   // Type keyword
    TK_MEMORYVIEW,  // Type keyword
    TK_NONETYPE,    // Type keyword
    // Identifier
    TK_IDENTIFIER,
    // Constants (Literals)
    TK_NUMBER,      // Represents int OR float literals initially
    TK_STRING,      // Represents non-bytes string literals
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
    TK_IMATMUL, // Use this for @= as per enum provided
    TK_WALNUT, // :=
    TK_FUNC_RETURN_TYPE, // ->
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
    NUMBER, // For TK_NUMBER, TK_COMPLEX
    STRING, // For TK_STRING, TK_BYTES
    PUNCTUATION,
    OPERATOR,
    EOFILE,
    UNKNOWN
};

// --- Token Structure ---
struct Token {
    TokenType type;
    string lexeme;
    int line;
    TokenCategory category;
};


//struct SymbolInfo {
//    string name;
//    string type = "unknown";
//    string value = "undefined";
//};


// --- Helper Functions ---
inline string tokenTypeToString(const TokenType type) {
    // Using the provided implementation...
    switch (type) {
        // Keywords (including type names when used as keywords)
        case TokenType::TK_IF: return "IF";
        case TokenType::TK_ELSE: return "ELSE";
        case TokenType::TK_FOR: return "FOR";
        case TokenType::TK_WHILE: return "WHILE";
        case TokenType::TK_DEF: return "DEF";
        case TokenType::TK_RETURN: return "RETURN";
        case TokenType::TK_FALSE: return "FALSE"; // Literal keyword
        case TokenType::TK_NONE: return "NONE";   // Literal keyword
        case TokenType::TK_TRUE: return "TRUE";   // Literal keyword
        case TokenType::TK_AND: return "AND";
        case TokenType::TK_AS: return "AS";
        case TokenType::TK_ASSERT: return "ASSERT";
        case TokenType::TK_ASYNC: return "ASYNC";
        case TokenType::TK_AWAIT: return "AWAIT";
        case TokenType::TK_BREAK: return "BREAK";
        case TokenType::TK_CLASS: return "CLASS";
        case TokenType::TK_CONTINUE: return "CONTINUE";
        case TokenType::TK_DEL: return "DEL";
        case TokenType::TK_ELIF: return "ELIF";
        case TokenType::TK_EXCEPT: return "EXCEPT";
        case TokenType::TK_FINALLY: return "FINALLY";
        case TokenType::TK_FROM: return "FROM";
        case TokenType::TK_GLOBAL: return "GLOBAL";
        case TokenType::TK_IMPORT: return "IMPORT";
        case TokenType::TK_IN: return "IN";
        case TokenType::TK_IS: return "IS";
        case TokenType::TK_LAMBDA: return "LAMBDA";
        case TokenType::TK_NONLOCAL: return "NONLOCAL";
        case TokenType::TK_NOT: return "NOT";
        case TokenType::TK_OR: return "OR";
        case TokenType::TK_PASS: return "PASS";
        case TokenType::TK_RAISE: return "RAISE";
        case TokenType::TK_TRY: return "TRY";
        case TokenType::TK_WITH: return "WITH";
        case TokenType::TK_YIELD: return "YIELD";
        case TokenType::TK_STR: return "STR_KEYWORD"; // Distinguish type keyword usage
        case TokenType::TK_INT: return "INT_KEYWORD";
        case TokenType::TK_FLOAT: return "FLOAT_KEYWORD";
        case TokenType::TK_COMPLEX: return "COMPLEX_KEYWORD_OR_LITERAL"; // Ambiguous here, context needed
        case TokenType::TK_LIST: return "LIST_KEYWORD";
        case TokenType::TK_TUPLE: return "TUPLE_KEYWORD";
        case TokenType::TK_RANGE: return "RANGE_KEYWORD";
        case TokenType::TK_DICT: return "DICT_KEYWORD";
        case TokenType::TK_SET: return "SET_KEYWORD";
        case TokenType::TK_FROZENSET: return "FROZENSET_KEYWORD";
        case TokenType::TK_BOOL: return "BOOL_KEYWORD";
        case TokenType::TK_BYTES: return "BYTES_KEYWORD_OR_LITERAL"; // Ambiguous here
        case TokenType::TK_BYTEARRAY: return "BYTEARRAY_KEYWORD";
        case TokenType::TK_MEMORYVIEW: return "MEMORYVIEW_KEYWORD";
        case TokenType::TK_NONETYPE: return "NONETYPE_KEYWORD";

            // Identifier
        case TokenType::TK_IDENTIFIER: return "IDENTIFIER";

            // Constants (Literals)
        case TokenType::TK_NUMBER: return "NUMBER_LITERAL"; // Int or Float
        case TokenType::TK_STRING: return "STRING_LITERAL"; // Normal string

            // Operators
        case TokenType::TK_PLUS: return "PLUS";
        case TokenType::TK_MINUS: return "MINUS";
        case TokenType::TK_MULTIPLY: return "MULTIPLY";
        case TokenType::TK_DIVIDE: return "DIVIDE";
        case TokenType::TK_FLOORDIV: return "FLOORDIV";
        case TokenType::TK_FLOORDIV_ASSIGN: return "FLOORDIV_ASSIGN";
        case TokenType::TK_MOD: return "MOD";
        case TokenType::TK_MOD_ASSIGN: return "MOD_ASSIGN";
        case TokenType::TK_POWER: return "POWER";
        case TokenType::TK_POWER_ASSIGN: return "POWER_ASSIGN";
        case TokenType::TK_BIT_AND: return "BIT_AND";
        case TokenType::TK_BIT_AND_ASSIGN: return "BIT_AND_ASSIGN";
        case TokenType::TK_BIT_OR: return "BIT_OR";
        case TokenType::TK_BIT_OR_ASSIGN: return "BIT_OR_ASSIGN";
        case TokenType::TK_BIT_XOR: return "BIT_XOR";
        case TokenType::TK_BIT_XOR_ASSIGN: return "BIT_XOR_ASSIGN";
        case TokenType::TK_BIT_NOT: return "BIT_NOT";
        case TokenType::TK_BIT_RIGHT_SHIFT: return "BIT_RIGHT_SHIFT";
        case TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN: return "BIT_RIGHT_SHIFT_ASSIGN";
        case TokenType::TK_BIT_LEFT_SHIFT: return "BIT_LEFT_SHIFT";
        case TokenType::TK_BIT_LEFT_SHIFT_ASSIGN: return "BIT_LEFT_SHIFT_ASSIGN";
        case TokenType::TK_ASSIGN: return "ASSIGN";
        case TokenType::TK_PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TokenType::TK_MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TokenType::TK_MULTIPLY_ASSIGN: return "MULTIPLY_ASSIGN";
        case TokenType::TK_DIVIDE_ASSIGN: return "DIVIDE_ASSIGN";
        case TokenType::TK_EQUAL: return "EQUAL";
        case TokenType::TK_NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::TK_GREATER: return "GREATER";
        case TokenType::TK_LESS: return "LESS";
        case TokenType::TK_GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::TK_LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::TK_MATMUL: return "MATMUL";
        case TokenType::TK_IMATMUL: return "MATMUL_ASSIGN"; // Assuming IMATMUL means Matmul Assign
        case TokenType::TK_WALNUT: return "WALNUT"; // :=
        case TokenType::TK_FUNC_RETURN_TYPE: return "FUNC_RETURN_TYPE"; // ->

            // Punctuation
        case TokenType::TK_LPAREN: return "LPAREN";
        case TokenType::TK_RPAREN: return "RPAREN";
        case TokenType::TK_LBRACKET: return "LBRACKET";
        case TokenType::TK_RBRACKET: return "RBRACKET";
        case TokenType::TK_LBRACE: return "LBRACE";
        case TokenType::TK_RBRACE: return "RBRACE";
        case TokenType::TK_COMMA: return "COMMA";
        case TokenType::TK_SEMICOLON: return "SEMICOLON";
        case TokenType::TK_COLON: return "COLON";
        case TokenType::TK_PERIOD: return "PERIOD";

            // End-of-file / Unknown
        case TokenType::TK_EOF: return "EOF";
        case TokenType::TK_UNKNOWN: return "UNKNOWN";
        default: return "INVALID_TOKEN_TYPE"; // Should not happen
    }
}


inline TokenCategory getTokenCategory(const TokenType type) {
    using TT = TokenType; // Alias for brevity
    switch (type) {
        // Keywords (Includes built-in type names when used as keywords/hints)
        case TT::TK_IF: case TT::TK_ELSE: case TT::TK_FOR: case TT::TK_WHILE: case TT::TK_DEF:
        case TT::TK_RETURN: case TT::TK_FALSE: case TT::TK_NONE: case TT::TK_TRUE: case TT::TK_AND:
        case TT::TK_AS: case TT::TK_ASSERT: case TT::TK_ASYNC: case TT::TK_AWAIT: case TT::TK_BREAK:
        case TT::TK_CLASS: case TT::TK_CONTINUE: case TT::TK_DEL: case TT::TK_ELIF: case TT::TK_EXCEPT:
        case TT::TK_FINALLY: case TT::TK_FROM: case TT::TK_GLOBAL: case TT::TK_IMPORT: case TT::TK_IN:
        case TT::TK_IS: case TT::TK_LAMBDA: case TT::TK_NONLOCAL: case TT::TK_NOT: case TT::TK_OR:
        case TT::TK_PASS: case TT::TK_RAISE: case TT::TK_TRY: case TT::TK_WITH: case TT::TK_YIELD:
        case TT::TK_STR: case TT::TK_INT: case TT::TK_FLOAT: /* TK_COMPLEX is ambiguous */ case TT::TK_LIST:
        case TT::TK_TUPLE: case TT::TK_RANGE: case TT::TK_DICT: case TT::TK_SET: case TT::TK_FROZENSET:
        case TT::TK_BOOL: /* TK_BYTES is ambiguous */ case TT::TK_BYTEARRAY: case TT::TK_MEMORYVIEW: case TT::TK_NONETYPE:
            return TokenCategory::KEYWORD;

            // Identifiers
        case TT::TK_IDENTIFIER:
            return TokenCategory::IDENTIFIER;

            // Constants / Literals
        case TT::TK_NUMBER: // int or float
        case TT::TK_COMPLEX: // complex literal
            return TokenCategory::NUMBER;
        case TT::TK_STRING: // Normal string literal
        case TT::TK_BYTES: // Bytes literal
            return TokenCategory::STRING;


            // Punctuation
        case TT::TK_LPAREN: case TT::TK_RPAREN: case TT::TK_LBRACKET: case TT::TK_RBRACKET: case TT::TK_LBRACE:
        case TT::TK_RBRACE: case TT::TK_COMMA: case TT::TK_SEMICOLON: case TT::TK_COLON: case TT::TK_PERIOD:
            return TokenCategory::PUNCTUATION;

            // Operators
        case TT::TK_PLUS: case TT::TK_MINUS: case TT::TK_MULTIPLY: case TT::TK_DIVIDE: case TT::TK_FLOORDIV:
        case TT::TK_FLOORDIV_ASSIGN: case TT::TK_MOD: case TT::TK_MOD_ASSIGN: case TT::TK_POWER:
        case TT::TK_POWER_ASSIGN: case TT::TK_BIT_AND: case TT::TK_BIT_AND_ASSIGN: case TT::TK_BIT_OR:
        case TT::TK_BIT_OR_ASSIGN: case TT::TK_BIT_XOR: case TT::TK_BIT_XOR_ASSIGN: case TT::TK_BIT_NOT:
        case TT::TK_BIT_RIGHT_SHIFT: case TT::TK_BIT_RIGHT_SHIFT_ASSIGN: case TT::TK_BIT_LEFT_SHIFT:
        case TT::TK_BIT_LEFT_SHIFT_ASSIGN: case TT::TK_ASSIGN: case TT::TK_PLUS_ASSIGN: case TT::TK_MINUS_ASSIGN:
        case TT::TK_MULTIPLY_ASSIGN: case TT::TK_DIVIDE_ASSIGN: case TT::TK_EQUAL: case TT::TK_NOT_EQUAL:
        case TT::TK_GREATER: case TT::TK_LESS: case TT::TK_GREATER_EQUAL: case TT::TK_LESS_EQUAL: case TT::TK_MATMUL:
        case TT::TK_IMATMUL: case TT::TK_WALNUT: case TT::TK_FUNC_RETURN_TYPE:
            return TokenCategory::OPERATOR;

            // End-of-file
        case TT::TK_EOF:
            return TokenCategory::EOFILE;

            // Unknown
        case TT::TK_UNKNOWN:
        default:
            return TokenCategory::UNKNOWN;
    }
}
