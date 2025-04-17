#include "Lexer.hpp"
#include <cctype>
#include <string>
#include <unordered_map>

using namespace std;

Lexer::Lexer(string input)
    : input(std::move(input)), pos(0), line(1), currentIndent(0), atLineStart(true) {
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
    // For debugging
    static int tokenCount = 0;
    tokenCount++;

    // If we have pending indentation tokens, return them first
    if (!pendingTokens.empty()) {
        Token token = pendingTokens.front();
        pendingTokens.erase(pendingTokens.begin());
        return token;
    }
    
    skipWhitespaceAndComments();
    
    // Re-check for pending tokens after processing indentation
    if (!pendingTokens.empty()) {
        Token token = pendingTokens.front();
        pendingTokens.erase(pendingTokens.begin());
        return token;
    }

    if (isAtEnd()) {
        // Before returning EOF, check if we need to emit DEDENT tokens
        if (!indentStack.empty()) {
            while (!indentStack.empty()) {
                currentIndent = indentStack.back();
                indentStack.pop_back();
                pendingTokens.push_back(createToken(TokenType::TK_DEDENT, "DEDENT"));
            }
            
            Token token = pendingTokens.front();
            pendingTokens.erase(pendingTokens.begin());
            return token;
        }
        
        // Add a newline before EOF if we're not already at the start of a line
        if (!atLineStart && currentIndent > 0) {
            atLineStart = true;
            
            // Generate DEDENT tokens to get back to level 0
            while (currentIndent > 0) {
                if (!indentStack.empty()) {
                    currentIndent = indentStack.back();
                    indentStack.pop_back();
                } else {
                    currentIndent = 0;
                }
                pendingTokens.push_back(createToken(TokenType::TK_DEDENT, "DEDENT"));
            }
            
            if (!pendingTokens.empty()) {
                Token token = pendingTokens.front();
                pendingTokens.erase(pendingTokens.begin());
                return token;
            }
        }
        
        return createToken(TokenType::TK_EOF, "");
    }

    const char currentCharacter = getCurrentCharacter();

    // Check for comments again, in case skipWhitespaceAndComments missed it
    // TODO: Re-check logic of skipWhitespaceAndComments, we might have to return next token every time
    if (currentCharacter == '#') {
        skipComment();
        return nextToken();
    }

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
    if (isAtEnd()) return '\0';
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

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = getCurrentCharacter();

        if (c == ' ' || c == '\t') {
            // Check if we're at the start of a line (for indentation)
            if (atLineStart) {
                processIndentation();
                break;
            }
            // Else skip whitespace in the middle of a line
            advanceToNextCharacter();
        } else if (c == '\n') {
            line++;
            advanceToNextCharacter();
            atLineStart = true; // Mark that we're at the start of a new line
        } else if (c == '\r') {
            advanceToNextCharacter();
        } else if (c == '#') {
            skipComment();
        } else {
            // If we're at the start of a line with non-whitespace, process for indentation
            if (atLineStart) {
                processIndentation();
            }
            break;
        }
    }
}

void Lexer::skipComment() {
    while (!isAtEnd() && getCurrentCharacter() != '\n') {
        advanceToNextCharacter();
    }
    // After a comment, check for updating new line
    if (!isAtEnd() && getCurrentCharacter() == '\n') {
        line++;
        advanceToNextCharacter();
        atLineStart = true;
    }
}

void Lexer::processIndentation() {
    int spaces = 0;

    // Count spaces or tabs at the beginning of the line
    while (!isAtEnd() && (getCurrentCharacter() == ' ' || getCurrentCharacter() == '\t')) {
        const char c = getCurrentCharacter();
        spaces += (c == '\t') ? 8 : 1;  // A tab is equivalent to 8 spaces in Python
        advanceToNextCharacter();
    }

    // If a line is empty or a comment, ignore indentation
    if (isAtEnd() || getCurrentCharacter() == '\n' || getCurrentCharacter() == '#') {
        return;
    }

    atLineStart = false;

    // Compare with the current indentation level
    if (spaces > currentIndent) {
        // Indent
        indentStack.push_back(currentIndent);
        currentIndent = spaces;
        pendingTokens.push_back(createToken(TokenType::TK_INDENT, "INDENT"));
    } else if (spaces < currentIndent) {
        // Dedent
        while (!indentStack.empty() && spaces < currentIndent) {
            currentIndent = indentStack.back();
            indentStack.pop_back();
            pendingTokens.push_back(createToken(TokenType::TK_DEDENT, "DEDENT"));
        }

        // Ensure indentation is consistent
        if (spaces != currentIndent) {
            // TODO: handle inconsistent indentation (error handling)
            // For now we just adjust to the current indentation
            currentIndent = spaces;
        }
    }
}

Token Lexer::handleIdentifierOrKeyword() {
    const size_t start = pos;
    while (!isAtEnd() && (isalnum(getCurrentCharacter()) || getCurrentCharacter() == '_')) {
        advanceToNextCharacter();
    }
    const string text = input.substr(start, pos - start);

    auto it = keywords.find(text);
    if (it != keywords.end()) {
        const TokenType type = it->second;
        return createToken(type, text);
    }
    if (!symbolTable.contains(text)) {
        SymbolInfo info;
        info.name = text;
        symbolTable[text] = info;
    }
    return createToken(TokenType::TK_IDENTIFIER, text);
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
    while (!isAtEnd() && getCurrentCharacter() != quote) {
        if (getCurrentCharacter() == '\\' && pos + 1 < input.size()) {
            advanceToNextCharacter();
        }
        advanceToNextCharacter();
    }

    if (isAtEnd()) {
        const string text = input.substr(start - 1, pos - (start - 1));
        return createToken(TokenType::TK_UNKNOWN, text);
    }

    advanceToNextCharacter();

    const string text = input.substr(start - 1, pos - (start - 1));
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
            return createToken(TokenType::TK_UNKNOWN, string(1, currentCharacter));
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

unordered_map<string, SymbolInfo>& Lexer::getSymbolTable() {
    return symbolTable;
}

const unordered_map<string, SymbolInfo>& Lexer::getSymbolTable() const {
    return symbolTable;
}