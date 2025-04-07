#include "Lexer.hpp"
#include <cctype>

Lexer::Lexer(std::string input)
    : input(std::move(input)), pos(0), line(1), column(1)
{
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
    };
}
Token Lexer::nextToken() {
        skipWhitespace();

        if (isAtEnd()) {
            return createToken(TokenType::TK_EOF, "");
        }

        const char currentCharacter = getCurrentCharacter();

        if (std::isalpha(currentCharacter) || currentCharacter == '_') {
            return handleIdentifierOrKeyword();
        }

        if (std::isdigit(currentCharacter)) {
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
    column++;
    return c;
}

bool Lexer::matchAndAdvance(const char expected) {
    if (isAtEnd() || input[pos] != expected)
        return false;
    pos++;
    column++;
    return true;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(getCurrentCharacter())) {
        if (getCurrentCharacter() == '\n') {
            line++;
            column = 0;
        }
        advanceToNextCharacter();
    }
}

Token Lexer::handleIdentifierOrKeyword() {
    const size_t start = pos;
    while (!isAtEnd() && (std::isalnum(getCurrentCharacter()) || getCurrentCharacter() == '_')) {
        advanceToNextCharacter();
    }
    const std::string text = input.substr(start, pos - start);
    if (keywords.contains(text)) {
        return Token{ keywords[text], text, line, column };
    }
    return Token{ TokenType::TK_IDENTIFIER, text, line, column };
}

Token Lexer::handleNumeric() {
    const size_t start = pos;
    while (!isAtEnd() && std::isdigit(getCurrentCharacter())) {
        advanceToNextCharacter();
    }
    if (!isAtEnd() && getCurrentCharacter() == '.') {
        advanceToNextCharacter();
        while (!isAtEnd() && std::isdigit(getCurrentCharacter())) {
            advanceToNextCharacter();
        }
    }
    const std::string text = input.substr(start, pos - start);
    return Token{ TokenType::TK_NUMBER, text, line, column };
}


Token Lexer::handleString() {
    const char quote = advanceToNextCharacter();
    const size_t start = pos;
    while (!isAtEnd() && getCurrentCharacter() != quote) {
        if (getCurrentCharacter() == '\\') {
            advanceToNextCharacter();
        }
        advanceToNextCharacter();
    }
    if (!isAtEnd()) {
        advanceToNextCharacter();
    }
    const std::string text = input.substr(start - 1, pos - start + 1);
    return Token{ TokenType::TK_STRING, text, line, column - 1 };
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
                return createToken(TokenType::TK_UNKNOWN, std::string(1, currentCharacter));
        }
    return createToken(TokenType::TK_UNKNOWN, std::string(1, currentCharacter));
}


Token Lexer::createToken(const TokenType type, const std::string &text) const {
    return Token{
        type,
        text,
        line,
        column - static_cast<int>(text.size()),
        getTokenCategory(type)
        };
}

// helper for single-character operators that might be part of an assignment operator
Token Lexer::operatorToken(const TokenType simpleType, const TokenType assignType, const char opChar) {
    advanceToNextCharacter();
    if (matchAndAdvance('=')) {
        std::string opStr;
        opStr.push_back(opChar);
        opStr.push_back('=');
        return createToken(assignType, opStr);
    }
    std::string opStr;
    opStr.push_back(opChar);
    return createToken(simpleType, opStr);
}