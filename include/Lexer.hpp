#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include "Token.hpp"

using namespace std;

class Lexer {
public:
    explicit Lexer(string input);

    Token nextToken();

    // getter for the symbol table
    const unordered_set<string>& getSymbolTable() const;

private:
    string input;
    size_t pos;
    int line;
    unordered_map<string, TokenType> keywords;
    unordered_set<string> symbolTable;

    bool isAtEnd() const;

    char getCurrentCharacter() const;

    char advanceToNextCharacter();

    bool matchAndAdvance(char expected);

    void skipWhitespaceAndComments();

    void skipComment();

    Token createToken(TokenType type, const string &text) const;

    Token handleIdentifierOrKeyword();

    Token handleNumeric();

    Token handleString();

    Token handleSymbol();

    Token operatorToken(TokenType simpleType, TokenType assignType, char opChar);
};

#endif