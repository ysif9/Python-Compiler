#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include "Token.hpp"

using namespace std;

class Lexer {
public:
    explicit Lexer(string input);

    Token nextToken();

    unordered_map<string, SymbolInfo>& getSymbolTable();

private:
    
    string input;
    size_t pos;
    int line;
    unordered_map<string, TokenType> keywords;

    unordered_map<string, SymbolInfo> symbolTable;
    
    // Indentation tracking
    vector<int> indentStack;
    int currentIndent;
    bool atLineStart;
    vector<Token> pendingTokens; // For storing DEDENT tokens


    bool isAtEnd() const;

    char getCurrentCharacter() const;

    char advanceToNextCharacter();

    bool matchAndAdvance(char expected);

    void skipWhitespaceAndComments();

    void skipComment();

    void processIndentation();
    
    Token createToken(TokenType type, const string &text) const;

    Token handleIdentifierOrKeyword();

    Token handleNumeric();

    Token handleString();

    Token handleSymbol();

    Token operatorToken(TokenType simpleType, TokenType assignType, char opChar);

    const unordered_map<string, SymbolInfo>& getSymbolTable() const;


};

#endif // LEXER_HPP