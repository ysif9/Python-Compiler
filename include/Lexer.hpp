#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include "Token.hpp" // Include the provided Token header

using namespace std;

// getTokenCategory is defined inline in Token.hpp, no forward declaration needed


struct Lexer_error {
    string message;
    int line;
    string lexeme;
};

class Lexer {

    vector<Lexer_error> errors;

public:
    explicit Lexer(string input);
    Token nextToken(); // Generates tokens one by one
    const unordered_map<string, string>& getSymbolTable(); // Gets the table *after* processing
    void processIdentifierTypes(); // Processes the generated tokens list
    vector<Token> tokens; // Public vector to store generated token

    // getter for the symbol table
    const vector<Lexer_error>& getErrors() const;
    string panicRecovery();

    static bool isKnownSymbol(char c);

    void reportError(const string &message, const string &lexeme);

private:
    string input;
    size_t pos;
    int line;
    unordered_map<string, TokenType> keywords;
    unordered_map<string, string> symbolTable; // Internal symbol table: <name, inferred_type_string>

    // Indentation tracking
    vector<int> indentStack;
    int currentIndent;
    bool atLineStart;
    vector<Token> pendingTokens; // For storing DEDENT tokens

    // Helper methods
    bool isAtEnd() const;

    char getCurrentCharacter() const;

    char advanceToNextCharacter();

    bool matchAndAdvance(char expected);

    bool skipMultilineComment();

    void skipWhitespaceAndComments();

    void skipComment();
    void processIndentation();
    Token createToken(TokenType type, const string &text) const;

    // Token handling methods
    Token handleIdentifierOrKeyword();

    Token handleNumeric();

    Token handleString();

    Token handleSymbol();

    Token operatorToken(TokenType simpleType, TokenType assignType, char opChar);

    // Type inference methods (called by processIdentifierTypes)
    string inferType(size_t& index); // Main inference function
    string inferListType(size_t& index);
    string inferTupleType(size_t& index);
    string inferDictOrSetType(size_t& index);
    string combineTypes(const vector<string>& types); // Helper to combine element types
    // Removed internal tokenTypeToString, use the one from Token.hpp
    // Removed inferComplexTypeHint
};

#endif // LEXER_HPP
