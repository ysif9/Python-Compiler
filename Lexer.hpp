#ifndef LEXER_HPP
#define LEXER_HPP


#include <string>
#include <unordered_map>
#include "Token.hpp"


class Lexer {
public:
    explicit Lexer(std::string input);

    Token nextToken();

private:
    std::string input;
    size_t pos;
    int line;
    int column;
    std::unordered_map<std::string, TokenType> keywords;

    bool isAtEnd() const;

    char getCurrentCharacter() const;

    char advanceToNextCharacter();

    bool matchAndAdvance(char expected);

    void skipWhitespace();

    Token createToken(TokenType type, const std::string &text) const;

    Token handleIdentifierOrKeyword();

    Token handleNumeric();

    Token handleString();

    Token handleSymbol();

    Token operatorToken(TokenType simpleType, TokenType assignType, char opChar);
};

#endif
