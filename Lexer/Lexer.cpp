#include "Lexer.hpp"
#include <cctype>
using namespace std;

Lexer::Lexer(string input)
    : input(std::move(input)), pos(0), line(1) {
    keywords = {
        {"if", TokenType::TK_IF},
        {"else", TokenType::TK_ELSE},
        {"for", TokenType::TK_FOR},
        {"while", TokenType::TK_WHILE},
        {"def", TokenType::TK_DEF},
        {"return", TokenType::TK_RETURN},
        {"False", TokenType::TK_FALSE},
        {"None", TokenType::TK_NONE},
        {"True", TokenType::TK_TRUE},
        {"and", TokenType::TK_AND},
        {"as", TokenType::TK_AS},
        {"assert", TokenType::TK_ASSERT},
        {"async", TokenType::TK_ASYNC},
        {"await", TokenType::TK_AWAIT},
        {"break", TokenType::TK_BREAK},
        {"class", TokenType::TK_CLASS},
        {"continue", TokenType::TK_CONTINUE},
        {"del", TokenType::TK_DEL},
        {"elif", TokenType::TK_ELIF},
        {"except", TokenType::TK_EXCEPT},
        {"finally", TokenType::TK_FINALLY},
        {"from", TokenType::TK_FROM},
        {"global", TokenType::TK_GLOBAL},
        {"import", TokenType::TK_IMPORT},
        {"in", TokenType::TK_IN},
        {"is", TokenType::TK_IS},
        {"lambda", TokenType::TK_LAMBDA},
        {"nonlocal", TokenType::TK_NONLOCAL},
        {"not", TokenType::TK_NOT},
        {"or", TokenType::TK_OR},
        {"pass", TokenType::TK_PASS},
        {"raise", TokenType::TK_RAISE},
        {"try", TokenType::TK_TRY},
        {"with", TokenType::TK_WITH},
        {"yield", TokenType::TK_YIELD},
        {"str", TokenType::TK_STR},
        {"int", TokenType::TK_INT},
        {"float", TokenType::TK_FLOAT},
        {"complex", TokenType::TK_COMPLEX},
        {"list", TokenType::TK_LIST},
        {"tuple", TokenType::TK_TUPLE},
        {"range", TokenType::TK_RANGE},
        {"dict", TokenType::TK_DICT},
        {"set", TokenType::TK_SET},
        {"frozenset", TokenType::TK_FROZENSET},
        {"bool", TokenType::TK_BOOL},
        {"bytes", TokenType::TK_BYTES},
        {"bytearray", TokenType::TK_BYTEARRAY},
        {"memoryview", TokenType::TK_MEMORYVIEW},
        {"NoneType", TokenType::TK_NONETYPE},
    };
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return createToken(TokenType::TK_EOF, "");
    }

    const char currentCharacter = getCurrentCharacter();

    if (isalpha(currentCharacter) || currentCharacter == '_') {
        return handleIdentifierOrKeyword();
    }

    if (isdigit(currentCharacter)) {
        return handleNumeric();
    }

    if (currentCharacter == '"' || currentCharacter == '\'') {
        return handleString();
    }

    return handleSymbol();
}

bool Lexer::isAtEnd() const {
    return pos >= input.size();
}

char Lexer::getCurrentCharacter() const {
    return isAtEnd() ? '\0' : input[pos];
}

char Lexer::advanceToNextCharacter() {
    const char c = input[pos];
    pos++;
    return c;
}

bool Lexer::matchAndAdvance(const char expected) {
    if (isAtEnd() || input[pos] != expected)
        return false;
    pos++;
    return true;
}

bool Lexer::skipQuoteComment() {
    const size_t start = pos;
    if (matchAndAdvance('"') && matchAndAdvance('"') && matchAndAdvance('"')) {
        while (!isAtEnd()) {
            if (getCurrentCharacter() == '"' &&
                pos + 2 < input.size() &&
                input[pos + 1] == '"' &&
                input[pos + 2] == '"') {

                advanceToNextCharacter(); // first "
                advanceToNextCharacter(); // second "
                advanceToNextCharacter(); // third "
                return true;
                }

            if (getCurrentCharacter() == '\n') {
                line++;
            }

            advanceToNextCharacter();
        }

        // If we reach here, the triple-quoted string was never closed
        string unterminated = input.substr(start, pos - start);
        reportError("Unterminated triple-quoted string", unterminated);
        return false;
    } else {
        pos = start; // rollback if not actually a triple quote
        return false;
    }
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = getCurrentCharacter();

        if (isspace(c)) {
            if (c == '\n') {
                line++;
            }
            advanceToNextCharacter();
        }
        else if (c == '#') {
            skipComment();
        }
        else if (c == '"') {
            if (!skipQuoteComment())
                break;
        }
        else {
            break;
        }
    }
}


void Lexer::skipComment() {
    while (!isAtEnd() && getCurrentCharacter() != '\n') {
        advanceToNextCharacter();  // advance characters until newline
    }
}


Token Lexer::handleIdentifierOrKeyword() {
    const size_t start = pos;
    while (!isAtEnd() && (isalnum(getCurrentCharacter()) || getCurrentCharacter() == '_')) {
        advanceToNextCharacter();
    }
    const string text = input.substr(start, pos - start);

    if (keywords.contains(text)) {
        TokenType type = keywords[text];
        return createToken(type, text);
    } else {
        // It's an identifier
        // Add it to the symbol table
        if (text.size() > 79) {
            reportError("Identifier name is too long", text);
            return createToken(TokenType::TK_UNKNOWN, text);
        }
        symbolTable.insert(text);
        // Create an identifier token
        return createToken(TokenType::TK_IDENTIFIER, text);
    }
}

Token Lexer::handleNumeric() {
    const size_t start = pos;
    while (!isAtEnd() && isdigit(getCurrentCharacter())) {
        advanceToNextCharacter();
    }
    if (!isAtEnd() && getCurrentCharacter() == '.') {
        // Peek ahead to ensure it's a digit after the dot, not just a standalone dot
        if (pos + 1 < input.size() && isdigit(input[pos + 1])) {
            advanceToNextCharacter(); // Consume the dot
            while (!isAtEnd() && isdigit(getCurrentCharacter())) {
                advanceToNextCharacter();
            }
        }
        // If it's just a dot followed by non-digit, treat the preceding digits as integer
        // and let the dot be handled as a separate token later.
    }
    const string text = input.substr(start, pos - start);
    return createToken(TokenType::TK_NUMBER, text);
}


Token Lexer::handleString() {
    const char quote = getCurrentCharacter();
    advanceToNextCharacter();
    const size_t start = pos;

    while (!isAtEnd()) {
        char c = getCurrentCharacter();

        if (c == '\n') {
            reportError("Unterminated string literal", input.substr(start - 1, pos - (start - 1)));
            return createToken(TokenType::TK_UNKNOWN, input.substr(start - 1, pos - (start - 1)));
        }

        if (c == quote) break;

        if (c == '\\' && pos + 1 < input.size()) {
            advanceToNextCharacter(); // skip the backslash
        }

        advanceToNextCharacter();
    }

    advanceToNextCharacter();
    const string text = input.substr(start, pos - (start - 1));

    return createToken(TokenType::TK_STRING, text);
}

Token Lexer::handleSymbol() {
    const char currentCharacter = getCurrentCharacter();
    switch (currentCharacter) {
        case '(':
            advanceToNextCharacter();
            return createToken(TokenType::TK_LPAREN, "(");
        case ')':
            advanceToNextCharacter();
            return createToken(TokenType::TK_RPAREN, ")");
        case '[':
            advanceToNextCharacter();
            return createToken(TokenType::TK_LBRACKET, "[");
        case ']':
            advanceToNextCharacter();
            return createToken(TokenType::TK_RBRACKET, "]");
        case '{':
            advanceToNextCharacter();
            return createToken(TokenType::TK_LBRACE, "{");
        case '}':
            advanceToNextCharacter();
            return createToken(TokenType::TK_RBRACE, "}");
        case ',':
            advanceToNextCharacter();
            return createToken(TokenType::TK_COMMA, ",");
        case ';':
            advanceToNextCharacter();
            return createToken(TokenType::TK_SEMICOLON, ";");
        case ':':
            return operatorToken(TokenType::TK_COLON, TokenType::TK_WALNUT, ':');
        case '.':
            advanceToNextCharacter();
            return createToken(TokenType::TK_PERIOD, ".");
        case '+':
            return operatorToken(TokenType::TK_PLUS, TokenType::TK_PLUS_ASSIGN, '+');
        case '-':
            advanceToNextCharacter();
            if (matchAndAdvance('>')) {
                return createToken(TokenType::TK_FUNC_RETURN_TYPE, "->");
            }
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_MINUS_ASSIGN, "-=");
            }
            return createToken(TokenType::TK_MINUS, "-");
        case '*':
            advanceToNextCharacter();
            if (matchAndAdvance('*')) {
                advanceToNextCharacter();
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_POWER_ASSIGN, "**=");
                }
                return createToken(TokenType::TK_POWER, "**");
            }
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_MULTIPLY_ASSIGN, "*=");
            }
            return createToken(TokenType::TK_MULTIPLY, "*");
        case '/':
            advanceToNextCharacter();
            if (matchAndAdvance('/')) {
                advanceToNextCharacter();
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_FLOORDIV_ASSIGN, "//=");
                }
                return createToken(TokenType::TK_FLOORDIV, "//");
            }
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_DIVIDE_ASSIGN, "/=");
            }
            return createToken(TokenType::TK_DIVIDE, "/");
        case '%':
            return operatorToken(TokenType::TK_MOD, TokenType::TK_MOD_ASSIGN, '%');
        case '@':
            return operatorToken(TokenType::TK_MATMUL, TokenType::TK_IMATMUL, '@');
        case '&':
            return operatorToken(TokenType::TK_BIT_AND, TokenType::TK_BIT_AND_ASSIGN, '&');
        case '|':
            return operatorToken(TokenType::TK_BIT_OR, TokenType::TK_BIT_OR_ASSIGN, '|');
        case '^':
            return operatorToken(TokenType::TK_BIT_XOR, TokenType::TK_BIT_XOR_ASSIGN, '^');
        case '~':
            advanceToNextCharacter();
            return createToken(TokenType::TK_BIT_NOT, "~");
        case '=':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_EQUAL, "==");
            }
            return createToken(TokenType::TK_ASSIGN, "=");
        case '!':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_NOT_EQUAL, "!=");
            }
            break;
        case '>':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_GREATER_EQUAL, ">=");
            }
            if (matchAndAdvance('>')) {
                advanceToNextCharacter();
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN, ">>=");
                }
                return createToken(TokenType::TK_BIT_RIGHT_SHIFT, ">>");
            }
            return createToken(TokenType::TK_GREATER, ">");
        case '<':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_LESS_EQUAL, "<=");
            }
            if (matchAndAdvance('<')) {
                advanceToNextCharacter();
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN, "<<=");
                }
                return createToken(TokenType::TK_BIT_LEFT_SHIFT, "<<");
            }
            return createToken(TokenType::TK_LESS, "<");
        default:
            advanceToNextCharacter();
            string unknown  = panicRecovery();
            return createToken(TokenType::TK_UNKNOWN, unknown);
    }

    return createToken(TokenType::TK_UNKNOWN, string(1, currentCharacter));
}


Token Lexer::createToken(const TokenType type, const string &text) const {
    return Token{
        type,
        text,
        line,
        getTokenCategory(type)
    };
}

// helper for single-character operators that might be part of an assignment operator
Token Lexer::operatorToken(const TokenType simpleType, const TokenType assignType, const char opChar) {
    advanceToNextCharacter();
    if (matchAndAdvance('=')) {
        string opStr;
        opStr.push_back(opChar);
        opStr.push_back('=');
        return createToken(assignType, opStr);
    }
    string opStr;
    opStr.push_back(opChar);
    return createToken(simpleType, opStr);
}
const unordered_set<string>& Lexer::getSymbolTable() const {
    return symbolTable;
}
const vector<Lexer_error>& Lexer::getErrors() const {
    return errors;
}

//skips unknown symbols
string Lexer::panicRecovery() {
    string unknown;
    while (!isAtEnd()) {
        char c = getCurrentCharacter();

        // recovery points: whitespace, known starting characters
        if (isspace(c) || isalpha(c) || isdigit(c) || c == '_' || isKnownSymbol(c)) {
            break;
        }
        unknown.push_back(c);
        advanceToNextCharacter();
    }
    reportError("Unknown Symbols found", unknown);
    return unknown;
}
bool Lexer::isKnownSymbol(char c) const {
    static const std::string knownSymbols = "[]{}(),.:;+-*/%&|^~!=<>\"\'";
    return knownSymbols.find(c) != std::string::npos;
}


void Lexer::reportError(const string& message, const string& lexeme) {
    errors.push_back({message, line, lexeme});
}


