#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <unordered_map>
#include <cctype>
#include <filesystem>
#include "token.hpp"


class Lexer {
public:
    explicit Lexer(std::string input)
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

    // main loop
    Token nextToken() {
        skipWhitespace();

        if (isAtEnd()) {
            return createToken(TokenType::TK_EOF, "");
        }

        const char currentCharacter = getCurrentCharacter();

        if (std::isalpha(currentCharacter) || currentCharacter == '_') {
            return handleIdentifier();
        }

        if (std::isdigit(currentCharacter)) {
            return handleNumeric();
        }

        if (currentCharacter == '"' || currentCharacter == '\'') {
            return handleString();
        }

        switch (currentCharacter) {
            case '(':
                advance();
                return createToken(TokenType::TK_LPAREN, "(");
            case ')':
                advance();
                return createToken(TokenType::TK_RPAREN, ")");
            case '[':
                advance();
                return createToken(TokenType::TK_LBRACKET, "[");
            case ']':
                advance();
                return createToken(TokenType::TK_RBRACKET, "]");
            case '{':
                advance();
                return createToken(TokenType::TK_LBRACE, "{");
            case '}':
                advance();
                return createToken(TokenType::TK_RBRACE, "}");
            case ',':
                advance();
                return createToken(TokenType::TK_COMMA, ",");
            case ';':
                advance();
                return createToken(TokenType::TK_SEMICOLON, ";");
            case ':':
                operatorToken(TokenType::TK_COLON, TokenType::TK_WALNUT, ':');
            case '.':
                advance();
                return createToken(TokenType::TK_PERIOD, ".");


            // Operators
            case '+':
                return operatorToken(TokenType::TK_PLUS, TokenType::TK_PLUS_ASSIGN, '+');
            case '-':
                advance();
                if (match('>')) {
                    return createToken(TokenType::TK_FUNC_RETURN_TYPE, "->");
                }
                if (match('=')) {
                    return createToken(TokenType::TK_MINUS_ASSIGN, "-=");
                }
                return createToken(TokenType::TK_MINUS, "-");
            case '*':
                advance();
                if (match('*')) {
                    advance();
                    if (match('=')) {
                        return createToken(TokenType::TK_POWER_ASSIGN, "**=");
                    }
                    return createToken(TokenType::TK_POWER, "**");
                }
                if (match('=')) {
                    return createToken(TokenType::TK_MULTIPLY_ASSIGN, "*=");
                }
                return createToken(TokenType::TK_MULTIPLY, "*");
            case '/':
                advance();
                if (match('/')) {
                    advance();
                    if (match('=')) {
                        return createToken(TokenType::TK_FLOORDIV_ASSIGN, "//=");
                    }
                    return createToken(TokenType::TK_FLOORDIV, "//");
                }
                if (match('=')) {
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
                advance();
                return createToken(TokenType::TK_BIT_NOT, "~");
            case '=':
                advance();
                if (match('=')) {
                    return createToken(TokenType::TK_EQUAL, "==");
                }
                return createToken(TokenType::TK_ASSIGN, "=");
            case '!':
                advance();
                if (match('=')) {
                    return createToken(TokenType::TK_NOT_EQUAL, "!=");
                }
                break;
            case '>':
                advance();
                if (match('=')) {
                    return createToken(TokenType::TK_GREATER_EQUAL, ">=");
                }
                if (match('>')) {
                    advance();
                    if (match('=')) {
                        return createToken(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN, ">>=");
                    }
                    return createToken(TokenType::TK_BIT_RIGHT_SHIFT, ">>");
                }
                return createToken(TokenType::TK_GREATER, ">");
            case '<':
                advance();
                if (match('=')) {
                    return createToken(TokenType::TK_LESS_EQUAL, "<=");
                }
                if (match('<')) {
                    advance();
                    if (match('=')) {
                        return createToken(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN, "<<=");
                    }
                    return createToken(TokenType::TK_BIT_LEFT_SHIFT, "<<");
                }
                return createToken(TokenType::TK_LESS, "<");
            default:
                advance();
                return createToken(TokenType::TK_UNKNOWN, std::string(1, currentCharacter));
        }
        return createToken(TokenType::TK_UNKNOWN, std::string(1, currentCharacter));
    }

private:
    std::string input;
    size_t pos;
    int line;
    int column;
    std::unordered_map<std::string, TokenType> keywords;

    bool isAtEnd() const { return pos >= input.size(); }

    char getCurrentCharacter() const { return isAtEnd() ? '\0' : input[pos]; }

    char getNextCharacter() const { return (pos + 1 < input.size()) ? input[pos + 1] : '\0'; }

    char advance() {
        const char c = input[pos];
        pos++;
        column++;
        return c;
    }

    bool match(const char expected) {
        if (isAtEnd() || input[pos] != expected)
            return false;
        pos++;
        column++;
        return true;
    }

    Token createToken(const TokenType type, const std::string &text) const {
        return Token{
         type,
         text,
         line,
         column - static_cast<int>(text.size())
         };
    }

    void skipWhitespace() {
        while (!isAtEnd() && std::isspace(getCurrentCharacter())) {
            if (getCurrentCharacter() == '\n') {
                line++;
                column = 0;
            }
            advance();
        }
    }

    Token handleIdentifier() {
        const size_t start = pos;
        int startColumn = column;
        while (!isAtEnd() && (std::isalnum(getCurrentCharacter()) || getCurrentCharacter() == '_')) {
            advance();
        }
        const std::string text = input.substr(start, pos - start);
        // If the identifier matches a keyword
        if (keywords.contains(text)) {
            return Token{ keywords[text], text, line, startColumn };
        }
        return Token{ TokenType::TK_IDENTIFIER, text, line, startColumn };
    }

    Token handleNumeric() {
        const size_t start = pos;
        int startColumn = column;
        while (!isAtEnd() && std::isdigit(getCurrentCharacter())) {
            advance();
        }
        if (!isAtEnd() && getCurrentCharacter() == '.') {
            advance();
            while (!isAtEnd() && std::isdigit(getCurrentCharacter())) {
                advance();
            }
        }
        const std::string text = input.substr(start, pos - start);
        return Token{ TokenType::TK_NUMBER, text, line, startColumn };
    }


    Token handleString() {
        const char quote = advance();
        const size_t start = pos;
        int startColumn = column;
        while (!isAtEnd() && getCurrentCharacter() != quote) {
            if (getCurrentCharacter() == '\\') {
                advance();
            }
            advance();
        }
        if (!isAtEnd()) {
            advance();
        }
        const std::string text = input.substr(start - 1, pos - start + 1);
        return Token{ TokenType::TK_STRING, text, line, startColumn - 1 };
    }

    // helper for single-character operators that might be part of an assignment operator
    Token operatorToken(const TokenType simpleType, const TokenType assignType, const char opChar) {
        advance();
        if (match('=')) {
            std::string opStr;
            opStr.push_back(opChar);
            opStr.push_back('=');
            return createToken(assignType, opStr);
        }
        std::string opStr;
        opStr.push_back(opChar);
        return createToken(simpleType, opStr);
    }
};

int main() {
    const std::string filename = "../test.py";

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string input = buffer.str();

    Lexer lexer(input);
    Token token;
    do {
        token = lexer.nextToken();
        std::cout << "<" << tokenTypeToString(token.type) << ", " << token.lexeme << ">" << std::endl;
    } while (token.type != TokenType::TK_EOF);

    return 0;
}
